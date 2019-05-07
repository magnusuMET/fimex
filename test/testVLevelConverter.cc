/*
 * Fimex, testVLevelConverter.cc
 *
 * (C) Copyright 2015, met.no
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
 *  Created on: Feb 25, 2015
 */

#include "testinghelpers.h"

#include "fimex/CDM.h"
#include "fimex/CDMFileReaderFactory.h"
#include "fimex/CDMReader.h"
#include "fimex/CDMVerticalInterpolator.h"
#include "fimex/coordSys/CoordinateSystem.h"
#include "fimex/coordSys/verticalTransform/ToVLevelConverter.h"
#include "fimex/coordSys/verticalTransform/VerticalTransformationUtils.h"
#include "fimex/Data.h"
#include "fimex/Logger.h"
#include "fimex/SliceBuilder.h"

#include <memory>

using namespace MetNoFimex;

TEST4FIMEX_TEST_CASE(test_pressure_integrator)
{
    const std::string fileName = pathTest("testdata_arome_vc.nc");

    typedef std::shared_ptr<const VerticalTransformation> VerticalTransformation_cp;
    typedef std::shared_ptr<ToVLevelConverter> ToVLevelConverter_p;

    CDMReader_p reader(CDMFileReaderFactory::create(MIFI_FILETYPE_NETCDF, fileName));
    CoordinateSystem_cp cs = findCompleteCoordinateSystemFor(MetNoFimex::listCoordinateSystems(reader), "x_wind_ml");
    TEST4FIMEX_REQUIRE(cs);

    VerticalTransformation_cp vt = cs->getVerticalTransformation();
    TEST4FIMEX_REQUIRE(vt);

    ToVLevelConverter_p pressc = vt->getConverter(reader, MIFI_VINT_PRESSURE, 0, cs);
    TEST4FIMEX_REQUIRE(pressc);

    const std::vector<double> pressures = (*pressc)(1, 0, 0);
    TEST4FIMEX_REQUIRE(pressures.size() == 65);
    TEST4FIMEX_CHECK_CLOSE(10, pressures[0], 1);
    TEST4FIMEX_CHECK_CLOSE(980, pressures[64], 1);

    if (1) {
        ToVLevelConverter_p altic = vt->getConverter(reader, MIFI_VINT_ALTITUDE, 0, cs);
        TEST4FIMEX_REQUIRE(altic);

        const std::vector<double> altitudes = (*altic)(1, 0, 0);
        TEST4FIMEX_REQUIRE(altitudes.size() == 65);
        TEST4FIMEX_CHECK_CLOSE(29910, altitudes[0], 1);
        TEST4FIMEX_CHECK_CLOSE(23210, altitudes[1], 1);
        TEST4FIMEX_CHECK_CLOSE(198, altitudes[63], 1);
        TEST4FIMEX_CHECK_CLOSE(173, altitudes[64], 1);
    }

    {
        VerticalConverter_p altivc = vt->getConverter(reader, cs, MIFI_VINT_ALTITUDE);
        TEST4FIMEX_REQUIRE(altivc);

        const std::vector<std::string> shape_ac = altivc->getShape();
        TEST4FIMEX_REQUIRE(4 == shape_ac.size());
        TEST4FIMEX_CHECK_EQ("x", shape_ac[0]);
        TEST4FIMEX_CHECK_EQ("y", shape_ac[1]);
        TEST4FIMEX_CHECK_EQ("hybrid", shape_ac[2]);
        TEST4FIMEX_CHECK_EQ("time", shape_ac[3]);

        SliceBuilder sb = createSliceBuilder(reader->getCDM(), altivc);
        sb.setStartAndSize("x", 1, 1);
        sb.setStartAndSize("time", 0, 1);
        if (1) {
            sb.setStartAndSize("hybrid", 0, 2);
            DataPtr vd = altivc->getDataSlice(sb);
            TEST4FIMEX_REQUIRE(vd);
            TEST4FIMEX_REQUIRE(2 == vd->size());
            shared_array<float> va = vd->asFloat();
            TEST4FIMEX_REQUIRE(va);
            TEST4FIMEX_CHECK_CLOSE(29910, va[0], 1);
            TEST4FIMEX_CHECK_CLOSE(23210, va[1], 1);
        }
        if (1) {
            sb.setStartAndSize("hybrid", 63, 2);
            DataPtr vd = altivc->getDataSlice(sb);
            TEST4FIMEX_REQUIRE(vd);
            TEST4FIMEX_REQUIRE_EQ(2, vd->size());
            shared_array<float> va = vd->asFloat();
            TEST4FIMEX_REQUIRE(va);
            TEST4FIMEX_CHECK_CLOSE(198, va[0], 1);
            TEST4FIMEX_CHECK_CLOSE(173, va[1], 1);
        }
    }
}

/* Create a test case where lowest altitude = highest pressure is first along the pressure axis.
 * Done by vertical interpolation of an existing test data set.
 */
TEST4FIMEX_TEST_CASE(test_pressure_integrator_up)
{
    const std::string fileName = pathTest("testdata_arome_vc.nc");
    CDMReader_p ncreader(CDMFileReaderFactory::create(MIFI_FILETYPE_NETCDF, fileName));

    std::vector<double> vi_level1;
    vi_level1.push_back(1000);
    vi_level1.push_back(850);
    vi_level1.push_back(500);
    vi_level1.push_back(300);
    vi_level1.push_back(100);
    vi_level1.push_back(50);
    std::shared_ptr<CDMVerticalInterpolator> reader = std::make_shared<CDMVerticalInterpolator>(ncreader, "pressure", "log");
    reader->interpolateToFixed(vi_level1);

    CoordinateSystem_cp cs = findCompleteCoordinateSystemFor(MetNoFimex::listCoordinateSystems(reader), "air_temperature_ml");
    TEST4FIMEX_REQUIRE(cs);

    std::shared_ptr<const VerticalTransformation> vt = cs->getVerticalTransformation();
    TEST4FIMEX_REQUIRE(vt);

    VerticalConverter_p altivc = vt->getConverter(reader, cs, MIFI_VINT_ALTITUDE);
    TEST4FIMEX_REQUIRE(altivc);

    const std::vector<std::string> shape_ac = altivc->getShape();
    TEST4FIMEX_REQUIRE(4 == shape_ac.size());
    TEST4FIMEX_CHECK_EQ("x", shape_ac[0]);
    TEST4FIMEX_CHECK_EQ("y", shape_ac[1]);
    TEST4FIMEX_CHECK_EQ("pressure", shape_ac[2]);
    TEST4FIMEX_CHECK_EQ("time", shape_ac[3]);

    SliceBuilder sb = createSliceBuilder(reader->getCDM(), altivc);
    sb.setStartAndSize("x", 0, 2);
    sb.setStartAndSize("y", 0, 1);
    sb.setStartAndSize("pressure", 0, 3);
    sb.setStartAndSize("time", 0, 1);
    DataPtr vd = altivc->getDataSlice(sb);
    TEST4FIMEX_REQUIRE(vd);
    TEST4FIMEX_REQUIRE_EQ(6, vd->size());
    shared_array<float> va = vd->asFloat();
    TEST4FIMEX_REQUIRE(va);
    TEST4FIMEX_CHECK_CLOSE(12.1, va[0], 1);
    TEST4FIMEX_CHECK_CLOSE(12.4, va[1], 1);
    TEST4FIMEX_CHECK_CLOSE(1292, va[2], 1);
    TEST4FIMEX_CHECK_CLOSE(1292, va[3], 1);
    TEST4FIMEX_CHECK_CLOSE(5000, va[4], 1);
    TEST4FIMEX_CHECK_CLOSE(5000, va[5], 1);
}
