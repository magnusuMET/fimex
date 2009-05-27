/*
 * Fimex
 *
 * (C) Copyright 2009, met.no
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
 */

#include "fimex/config.h"
#include <boost/version.hpp>
#if defined(HAVE_BOOST_UNIT_TEST_FRAMEWORK) && (BOOST_VERSION >= 103400)

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
using boost::unit_test_framework::test_suite;

#include <iostream>
#include <fstream>
#include "fimex/FeltCDMReader.h"
#include "fimex/CDMQualityExtractor.h"

using namespace std;
using namespace MetNoFimex;

//#define TEST_DEBUG

BOOST_AUTO_TEST_CASE( test_qualityExtract )
{
	string topSrcDir(TOP_SRCDIR);
	string fileName(topSrcDir+"/test/flth00.dat");
	if (!ifstream(fileName.c_str())) {
		// no testfile, skip test
		return;
	}
	boost::shared_ptr<CDMReader> feltReader(new FeltCDMReader(fileName, topSrcDir + "/share/etc/felt2nc_variables.xml"));
	boost::shared_ptr<CDMQualityExtractor> extract(new CDMQualityExtractor(feltReader, "", topSrcDir + "/share/etc/cdmQualityConfig.xml"));

	map<string, string> statusVariables = extract->getStatusVariable();
	map<string, string> variableFlags = extract->getVariableFlags();
	map<string, vector<double> > variableValues = extract->getVariableValues();
#ifdef TEST_DEBUG
	for (map<string, string>::iterator svIt = statusVariables.begin(); svIt != statusVariables.end(); ++svIt) {
	    string varName = svIt->first;
	    string statusVarName = svIt->second;
        cerr << varName << ": " << statusVarName;
	    if (variableFlags.find(varName) != variableFlags.end()) {
	        cerr << ": flag: " << variableFlags[varName];
	    }
	    if (variableValues.find(varName) != variableValues.end()) {
	        cerr << ": values: ";
	        vector<double> vals = variableValues[varName];
	        for (size_t i = 0; i < vals.size(); i++) {
	            cerr << i << ", ";
	        }
	    }
	    cerr << endl;
	}
#endif /* TEST_DEBUG */
    BOOST_CHECK(statusVariables["bla"] == "blub");
    BOOST_CHECK(variableFlags.find("bla") == variableFlags.end());
    BOOST_CHECK(variableValues["bla"][2] == 3);
}

#else
// no boost testframework
int main(int argc, char* args[]) {
}
#endif
