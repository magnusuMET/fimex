/*
 * Fimex, OceanSG1.cc
 *
 * (C) Copyright 2013, met.no
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
 *  Created on: Aug 8, 2013
 *      Author: heikok
 */

#include "fimex/coordSys/verticalTransform/OceanSG1.h"
#include "fimex/coordSys/verticalTransform/OceanSG2.h"
#include "fimex/CDM.h"
#include "fimex/CDMReader.h"
#include "fimex/CDMReaderUtils.h"
#include "fimex/coordSys/CoordinateSystem.h"
#include "fimex/Data.h"
#include "fimex/Utils.h"
#include "fimex/coordSys/verticalTransform/ToVLevelConverter.h"
#include "fimex/coordSys/verticalTransform/VerticalTransformationUtils.h"
#include "fimex/vertical_coordinate_transformations.h"

#include "fimex/ArrayLoop.h"

#include <algorithm>
#include <functional>
#include <sstream>

namespace MetNoFimex {

static const double ZERO = 0;

OceanSCoordinateGToDepthConverter::OceanSCoordinateGToDepthConverter(CDMReaderPtr reader, CoordSysPtr cs,
                                                                     const OceanSGVars& vars, heightconversion_t func)
    : BasicVerticalConverter(reader, cs)
    , vars_(vars)
    , func_(func)
{
}

std::vector<std::string> OceanSCoordinateGToDepthConverter::getShape() const
{
    return ShapeMerger(reader_->getCDM(), cs_)
            .merge(vars_.depth)
            .merge(vars_.eta) // empty eta var name handled by ShapeMerger
            .merge(vars_.s)
            .shape();
}

DataPtr OceanSCoordinateGToDepthConverter::getDataSlice(const SliceBuilder& sb) const
{
    if (!vars_.isComplete())
        throw CDMException("OceanSCoordinateGToDepthConverter incomplete formula terms");

    const CDM& cdm = reader_->getCDM();

    DataPtr depth_cData = getSliceData(reader_, sb, vars_.depth_c, "m");
    const double depth_c = depth_cData ? depth_cData->asDouble()[0] : 0;

    VarDouble s(reader_,  vars_.s, "", sb);
    VarDouble c(reader_, vars_.C, "", sb);
    VarDouble depth(reader_, vars_.depth, "m", sb);

    ArrayDims dimsZ = makeArrayDims(sb);
    boost::shared_array<double> z_values(new double[dimsZ.volume()]);

    enum { S, C, DEPTH, Z, ETA }; // ETA must be last as it is optional
    ArrayGroup group = ArrayGroup().add(s.dims).add(c.dims).add(depth.dims).add(dimsZ);

    ArrayDims dimsEta;
    boost::shared_array<double> eta_values;
    if (!vars_.eta.empty()) {
        eta_values = getSliceDoubles(reader_, sb, vars_.eta, "m");
        dimsEta = makeArrayDims(adaptSliceBuilder(cdm, vars_.eta, sb));
        group.add(dimsEta);
    }

    Loop loop(group);
    do {
        const double eta = eta_values ? eta_values[loop[ETA]] : 0;
        func_(group.sharedVolume(), depth.values[loop[DEPTH]], depth_c, eta,
                &s.values[loop[S]], &c.values[loop[C]], &z_values[loop[Z]]);
    } while (loop.next());

    /* z as calculated by formulas is negative down, but we want positive down */
    std::transform(z_values.get(), z_values.get() + dimsZ.volume(), z_values.get(), std::bind1st(std::multiplies<double>(),-1.));

    // TODO transform invalid -> nan, nan -> bad?

    return createData(dimsZ.volume(), z_values);
}

std::vector<std::string> OceanSCoordinateGToDepthConverter::getValidityShape(const std::string& verticalDim) const
{
    const CDM& cdm = reader_->getCDM();
    std::vector<std::string> shd = cdm.getVariable(vars_.depth).getShape();
    std::replace(shd.begin(), shd.end(), cs_->getGeoZAxis()->getName(), verticalDim);
    if (std::find(shd.begin(), shd.end(), verticalDim) == shd.end())
        shd.push_back(verticalDim);
    return shd;
}

DataPtr OceanSCoordinateGToDepthConverter::getValiditySlice(const SliceBuilder& sb, const std::vector<double>& verticalValues) const
{
    const CDM& cdm = reader_->getCDM();
    const std::string& verticalDim = cs_->getGeoZAxis()->getName();

    const boost::shared_array<double> depthValues = getSliceDoubles(reader_, sb, vars_.depth, "m");
    ArrayDims dimsDepth = makeArrayDims(adaptSliceBuilder(cdm, vars_.depth, sb));

    ArrayDims dimsValid = makeArrayDims(cdm, getValidityShape(verticalDim));
    dimsValid.set_length(verticalDim, verticalValues.size());

    ArrayDims dimsVertical = ArrayDims().add(verticalDim, verticalValues.size());

    enum { DEPTH, VERTICAL, VALID };
    ArrayGroup group = ArrayGroup().add(dimsDepth).add(dimsVertical).add(dimsValid);
    group.minimizeShared(0);

    boost::shared_array<unsigned char> validValues(new unsigned char[dimsValid.volume()]);

    Loop loop(group);
    do {
        validValues[loop[VALID]] = (depthValues[loop[DEPTH]] >= verticalValues[loop[VERTICAL]]) ? 1 : 0;
    } while (loop.next());

    return createData(dimsValid.volume(), validValues);
}

} // namespace MetNoFimex