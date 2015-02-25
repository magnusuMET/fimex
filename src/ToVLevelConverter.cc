/*
 * Fimex, ToVLevelConverter.cc
 *
 * (C) Copyright 2011, met.no
 *
 * Project Info:  https://wiki.met.no/fimex/start
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 *  Created on: Aug 12, 2011
 *      Author: Heiko Klein
 */

#include "fimex/coordSys/verticalTransform/ToVLevelConverter.h"
#include <map>
#include <functional>
#include <algorithm>
#include <boost/regex.hpp>
#include "fimex/CDMReader.h"
#include "fimex/Data.h"
#include "fimex/CDM.h"
#include "fimex/Logger.h"
#include "fimex/interpolation.h"
#include "fimex/vertical_coordinate_transformations.h"
#include <fimex/coordSys/verticalTransform/VerticalTransformation.h>

namespace MetNoFimex {

using namespace std;

static LoggerPtr logger = getLogger("fimex.ToVLevelConverter");

bool ToVLevelConverter::isValid(double, size_t, size_t, size_t)
{
    return true;
}

vector<double> IdentityToVLevelConverter::operator()(size_t, size_t, size_t)
{
    return vlevel_;
}

vector<double> Identity4DToVLevelConverter::operator()(size_t x, size_t y, size_t t) {
    vector<double> h(nz_);
    for (size_t z = 0; z < nz_; z++)
        h.at(z) = pressure_[((t*nz_ + z)*ny_ + y)*nx_ +x];
    return h;
}

LnPressureToPressureConverter::LnPressureToPressureConverter(double p0, const vector<double>& lnP) : pres_(lnP.size())
{
    mifi_atmosphere_ln_pressure(lnP.size(), p0, &lnP[0], &pres_[0]);
}

vector<double> LnPressureToPressureConverter::operator()(size_t, size_t, size_t)
{
    return pres_;
}

AltitudeStandardToPressureConverter::AltitudeStandardToPressureConverter(const vector<double>& h) : pres_(h.size())
{
    mifi_barometric_standard_pressure(h.size(), &h[0], &pres_[0]);
}

vector<double> AltitudeStandardToPressureConverter::operator()(size_t, size_t, size_t)
{
    return pres_;
}

vector<double> SigmaToPressureConverter::operator()(size_t x, size_t y, size_t t) {
    vector<double> p(sigma_.size());
    mifi_atmosphere_sigma_pressure(sigma_.size(), ptop_, ps_[mifi_3d_array_position(x,y,t,nx_,ny_,nt_)], &sigma_[0], &p[0]);
    return p;
}

vector<double> HybridSigmaApToPressureConverter::operator()(size_t x, size_t y, size_t t) {
    vector<double> p(ap_.size());
    mifi_atmosphere_hybrid_sigma_ap_pressure(ap_.size(), ps_[mifi_3d_array_position(x,y,t,nx_,ny_,nt_)], &ap_[0], &b_[0], &p[0]);
    return p;
}


vector<double> HybridSigmaToPressureConverter::operator()(size_t x, size_t y, size_t t) {
    vector<double> p(a_.size());
    mifi_atmosphere_hybrid_sigma_pressure(a_.size(), p0_, ps_[mifi_3d_array_position(x,y,t,nx_,ny_,nt_)], &a_[0], &b_[0], &p[0]);
    return p;
}


vector<double> PressureToStandardAltitudeConverter::operator()(size_t x, size_t y, size_t t) {
    assert(presConv_.get() != 0);
    const vector<double> p((*presConv_)(x,y,t));
    vector<double> h(p.size());
    mifi_barometric_standard_altitude(p.size(), &p[0], &h[0]);
    return h;
}

PressureIntegrationToAltitudeConverter::PressureIntegrationToAltitudeConverter(boost::shared_ptr<ToVLevelConverter> presConv,
        float_cp sapVal, float_cp sgpVal, float_cp airtVal, float_cp shVal,
        size_t nx, size_t ny, size_t nt)
    : presConv_(presConv), surface_air_pressure_(sapVal),
      surface_geopotential_(sgpVal), air_temperature_(airtVal),
      specific_humidity_(shVal), nx_(nx), ny_(ny), nt_(nt)
{
    assert(presConv_);
    assert(surface_air_pressure_);
    assert(surface_geopotential_);
    assert(air_temperature_);
}

vector<double> PressureIntegrationToAltitudeConverter::operator()(size_t x, size_t y, size_t t)
{
    const vector<double> pressure((*presConv_)(x,y,t));
    const size_t nl = pressure.size();
    if (nl == 0)
        return vector<double>();

    int l0 = 0, l1 = nl-1, dl = 1;
    if (pressure.front() < pressure.back()) {
        std::swap(l0, l1);
        dl = -1;
    }
    l1 += dl;

    const size_t idx3 = mifi_3d_array_position(x,y,t,nx_,ny_,nt_);
    const float p_surf = surface_air_pressure_[idx3];

    double a = surface_geopotential_[idx3] / MIFI_EARTH_GRAVITY;

    vector<double> altitude(nl);
    for (int l = l0; l != l1; l += dl) {
        const float p_low_alti = (l == l0) ? p_surf : pressure.at(l - dl);
        const float p_high_alti = pressure.at(l);

        const size_t idx4 = mifi_3d_array_position(x,y,l,nx_,ny_,nl) + t * (nx_*ny_*nl*nt_);
        float Tv = air_temperature_[idx4];
        if (specific_humidity_)
            Tv = mifi_virtual_temperature(specific_humidity_[idx4], Tv);

        a += mifi_barometric_layer_thickness(p_low_alti, p_high_alti, Tv);
        altitude.at(l) = a;
    }
    return altitude;
}

vector<double> AltitudeConverterToHeightConverter::operator()(size_t x, size_t y, size_t t) {
    assert(conv_.get() != 0);
    vector<double> h = (*conv_)(x, y, t);
    for (size_t z = 0; z < nz_; z++) {
        float hg = h.at(z);
        h.at(z) =  static_cast<double>(hg - topo_[mifi_3d_array_position(x,y,t,nx_,ny_,nt_)]);
    }
    return h;
}

vector<double> HeightConverterToAltitudeConverter::operator()(size_t x, size_t y, size_t t) {
    assert(conv_.get() != 0);
    vector<double> alti = (*conv_)(x, y, t);
    for (size_t z = 0; z < nz_; z++) {
        float hg = alti.at(z);
        alti.at(z) =  static_cast<double>(hg + topo_[mifi_3d_array_position(x,y,t,nx_,ny_,nt_)]);
    }
    return alti;
}

vector<double> GeopotentialToAltitudeConverter::operator()(size_t x, size_t y, size_t t) {
    vector<double> h(nz_);
    for (size_t z = 0; z < nz_; z++) {
        float hg = geopot_[((t*nz_ + z)*ny_ + y)*nx_ +x];
        h.at(z) =  static_cast<double>(hg);
    }
    return h;
}



vector<double> OceanSCoordinateGToDepthConverter::operator()(size_t x, size_t y, size_t t) {
    vector<double> z(nz_);
    float depth, eta;
    if (timeDependentDepth_) {
        depth = depth_.getDouble(x,y,t);
    } else {
        depth = depth_.getDouble(x,y);
    }
    eta = eta_.getDouble(x,y,t);
    func_(nz_, depth, depth_c_, eta, &s_[0], &C_[0], &z[0]);
    /* z as calculated by formulas is negative down, but we want positive down */
    transform(z.begin(), z.end(), z.begin(), bind1st(multiplies<double>(),-1.));
    return z;
}
bool OceanSCoordinateGToDepthConverter::isValid(double vVal, size_t x, size_t y, size_t t) {
    float depth;
    if (timeDependentDepth_) {
        depth = depth_.getDouble(x,y,t);
    } else {
        depth = depth_.getDouble(x,y);
    }
    return (depth >= vVal);
}


}
