/***********************************************************************************************************************
*                                                                                                                      *
* libscopehal                                                                                                          *
*                                                                                                                      *
* Copyright (c) 2012-2026 Andrew D. Zonenberg and contributors                                                         *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

/**
	@file
	@brief Unit tests for Tektronix waveform preamble parsing (no instrument required)
 */
#ifdef _CATCH2_V3
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "../../lib/scopehal/scopehal.h"
#include "../../lib/scopehal/TektronixOscilloscope.h"

using namespace std;
using Catch::Approx;

//Preambles captured from a MDO3024 (firmware v1.30) via raw socket

//VERBose ON, DATa:WIDth 2, DATa:ENCdg RIBinary (MSB first), channel offset and position both zero
static const char* g_mdo3kPreambleVerbose16 =
	"2;16;BINARY;RI;MSB;\"Ch1, DC coupling, 100.0mV/div, 4.000us/div, 10000 points, Sample mode\";10000;Y;LINEAR;"
	"\"s\";4.0000E-9;-20.0000E-6;0;\"V\";15.6250E-6;0.0E+0;0.0E+0;TIME;ANALOG;0.0E+0;0.0E+0;0.0E+0";

//VERBose OFF, DATa:WIDth 1, DATa:ENCdg SRIbinary (LSB first), CH1:OFFSet 0.2 V, CH1:POSition 1 div
static const char* g_mdo3kPreambleTerse8 =
	"1;8;BIN;RI;LSB;\"Ch1, DC coupling, 100.0mV/div, 4.000us/div, 10000 points, Sample mode\";10000;Y;LINEA;"
	"\"s\";4.0000E-9;-20.0000E-6;0;\"V\";4.0000E-3;25.0000;200.0000E-3;TIM;ANALOG;0.0E+0;0.0E+0;0.0E+0";

//MSO5/6 style preamble (has an extra ASC_FMT field after BN_FMT) must be rejected by the MDO3000 parser
static const char* g_mso56Preamble =
	"1;8;BIN;RI;FP;LSB;\"Ch1, DC coupling, 100.0mV/div, 4.000us/div, 10000 points, Sample mode\";10000;Y;LINEAR;"
	"\"s\";4.0000E-9;-20.0000E-6;0;\"V\";4.0000E-3;0.0E+0;0.0E+0;TIME;ANALOG;0.0E+0;0.0E+0;0.0E+0";

TEST_CASE("TektronixMDO3000_Preamble_Verbose16Bit")
{
	string in(g_mdo3kPreambleVerbose16);
	TektronixOscilloscope::mso56_preamble p;
	REQUIRE(TektronixOscilloscope::ParsePreambleMDO3000(in, p));

	REQUIRE(p.byte_num == 2);
	REQUIRE(p.bit_num == 16);
	REQUIRE(string(p.encoding) == "BINARY");
	REQUIRE(string(p.bin_format) == "RI");
	REQUIRE(string(p.byte_order) == "MSB");
	REQUIRE(string(p.wfid) == "\"Ch1, DC coupling, 100.0mV/div, 4.000us/div, 10000 points, Sample mode\"");
	REQUIRE(p.nr_pt == 10000);
	REQUIRE(string(p.pt_fmt) == "Y");
	REQUIRE(string(p.pt_order) == "LINEAR");
	REQUIRE(string(p.xunit) == "\"s\"");
	REQUIRE(p.xincrement == Approx(4e-9));
	REQUIRE(p.xzero == Approx(-20e-6));
	REQUIRE(p.pt_off == 0);
	REQUIRE(string(p.yunit) == "\"V\"");
	REQUIRE(p.ymult == Approx(15.625e-6));
	REQUIRE(p.yoff == Approx(0));
	REQUIRE(p.yzero == Approx(0));
	REQUIRE(string(p.domain) == "TIME");
	REQUIRE(string(p.wfmtype) == "ANALOG");
	REQUIRE(p.centerfreq == Approx(0));
	REQUIRE(p.span == Approx(0));
}

TEST_CASE("TektronixMDO3000_Preamble_Terse8Bit")
{
	string in(g_mdo3kPreambleTerse8);
	TektronixOscilloscope::mso56_preamble p;
	REQUIRE(TektronixOscilloscope::ParsePreambleMDO3000(in, p));

	REQUIRE(p.byte_num == 1);
	REQUIRE(p.bit_num == 8);
	REQUIRE(string(p.encoding) == "BIN");
	REQUIRE(string(p.byte_order) == "LSB");
	REQUIRE(p.nr_pt == 10000);
	REQUIRE(p.xincrement == Approx(4e-9));
	REQUIRE(p.xzero == Approx(-20e-6));
	REQUIRE(p.ymult == Approx(4e-3));

	//YOFF is in digitizer levels (25 LSB/div * 1 div), YZERO is in volts (the channel offset)
	REQUIRE(p.yoff == Approx(25.0));
	REQUIRE(p.yzero == Approx(0.2));
	REQUIRE(string(p.domain) == "TIM");
}

TEST_CASE("TektronixMDO3000_Preamble_RejectsOtherFormats")
{
	TektronixOscilloscope::mso56_preamble p;

	string mso56(g_mso56Preamble);
	REQUIRE_FALSE(TektronixOscilloscope::ParsePreambleMDO3000(mso56, p));

	string truncated("1;8;BIN;RI;LSB;\"Ch1\";10000;Y;LINEA;\"s\";4.0000E-9");
	REQUIRE_FALSE(TektronixOscilloscope::ParsePreambleMDO3000(truncated, p));

	string empty;
	REQUIRE_FALSE(TektronixOscilloscope::ParsePreambleMDO3000(empty, p));
}
