/*
 * =============================================================================
 *   ROC Runtime Conformance Release License
 * =============================================================================
 * The University of Illinois/NCSA
 * Open Source License (NCSA)
 *
 * Copyright (c) 2019, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * Developed by:
 *
 *                 AMD Research and AMD ROC Software Development
 *
 *                 Advanced Micro Devices, Inc.
 *
 *                 www.amd.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal with the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimers.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimers in
 *    the documentation and/or other materials provided with the distribution.
 *  - Neither the names of <Name of Development Group, Name of Institution>,
 *    nor the names of its contributors may be used to endorse or promote
 *    products derived from this Software without specific prior written
 *    permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS WITH THE SOFTWARE.
 *
 */

#include <stdint.h>
#include <stddef.h>

#include <iostream>
#include <string>

#include "gtest/gtest.h"
#include "rocm_smi/rocm_smi.h"
#include "rocm_smi_test/functional/volt_freq_curv_read.h"
#include "rocm_smi_test/test_common.h"

TestVoltCurvRead::TestVoltCurvRead() : TestBase() {
  set_title("RSMI Voltage-Frequency Curve Read Test");
  set_description("The Voltage-Frequency Read tests verifies that the voltage"
                         " frequency curve information can be read properly.");
}

TestVoltCurvRead::~TestVoltCurvRead(void) {
}

void TestVoltCurvRead::SetUp(void) {
  TestBase::SetUp();

  return;
}

void TestVoltCurvRead::DisplayTestInfo(void) {
  TestBase::DisplayTestInfo();
}

void TestVoltCurvRead::DisplayResults(void) const {
  TestBase::DisplayResults();
  return;
}

void TestVoltCurvRead::Close() {
  // This will close handles opened within rsmitst utility calls and call
  // rsmi_shut_down(), so it should be done after other hsa cleanup
  TestBase::Close();
}

static void pt_rng_mhz(std::string title, rsmi_range *r) {
  assert(r != nullptr);

  std::cout << title << std::endl;
  std::cout << "\t\t** " << r->lower_bound/1000000 << " to " <<
                                r->upper_bound/1000000 << " MHz" << std::endl;
}

static void print_pnt(rsmi_od_vddc_point *pt) {
  std::cout << "\t\t** Frequency: " << pt->frequency/1000000 << "MHz" <<
                                                                    std::endl;
  std::cout << "\t\t** Voltage: " << pt->voltage << "mV" << std::endl;
}
static void pt_vddc_curve(rsmi_od_vddc_point *c) {
  assert(c != nullptr);

  for (uint32_t i = 0; i < RSMI_NUM_VOLTAGE_CURVE_POINTS; ++i) {
    print_pnt(&c[i]);
  }
}

static void print_rsmi_od_volt_freq_data(rsmi_od_volt_freq_data *odv) {
  assert(odv != nullptr);

  std::cout.setf(std::ios::dec, std::ios::basefield);
  pt_rng_mhz("\t\tCurrent SCLK frequency range:", &odv->curr_sclk_range);
  pt_rng_mhz("\t\tCurrent MCLK frequency range:", &odv->curr_mclk_range);
  pt_rng_mhz("\t\tMin/Max Possible SCLK frequency range:",
                                                      &odv->sclk_freq_limits);
  pt_rng_mhz("\t\tMin/Max Possible MCLK frequency range:",
                                                      &odv->mclk_freq_limits);

  std::cout << "\t\tCurrent Freq/Volt. curve:" << std::endl;
  pt_vddc_curve(odv->curve);

  std::cout << "\tNumber of Freq./Volt. regions: " <<
                                                odv->num_regions << std::endl;
}

static void print_odv_region(rsmi_freq_volt_region *region) {
  std::cout << "\t\t\"lower-left\" corner:" << std::endl;
  print_pnt(&region->min_corner);
  std::cout << "\t\t\"upper-right\" corner:" << std::endl;
  print_pnt(&region->max_corner);
}
static void print_rsmi_od_volt_freq_regions(uint32_t num_regions,
                                             rsmi_freq_volt_region *regions) {
  for (uint32_t i = 0; i < num_regions; ++i) {
    std::cout << "\tRegion " << i << ":" << std::endl;
    print_odv_region(&regions[i]);
  }
}

void TestVoltCurvRead::Run(void) {
  rsmi_status_t err;
  rsmi_od_volt_freq_data odv;

  TestBase::Run();

  for (uint32_t i = 0; i < num_monitor_devs(); ++i) {
    PrintDeviceHeader(i);

    err = rsmi_dev_od_volt_info_get(i, &odv);
    if (err == RSMI_STATUS_FILE_ERROR ||
                                   err == RSMI_STATUS_NOT_YET_IMPLEMENTED) {
      IF_VERB(STANDARD) {
        std::cout <<
            "\t**rsmi_dev_od_volt_info_get: Not supported on this machine"
                                                               << std::endl;
      }
    } else {
      CHK_ERR_ASRT(err)
    }

    if (err == RSMI_STATUS_SUCCESS) {
      std::cout << "\t**Frequency-voltage curve data:" << std::endl;
      print_rsmi_od_volt_freq_data(&odv);

      rsmi_freq_volt_region *regions;
      uint32_t num_regions;
      regions = new rsmi_freq_volt_region[odv.num_regions];
      ASSERT_TRUE(regions != nullptr);

      num_regions = odv.num_regions;
      err = rsmi_dev_od_volt_curve_regions_get(i, &num_regions, regions);
      CHK_ERR_ASRT(err)
      ASSERT_TRUE(num_regions == odv.num_regions);

      std::cout << "\t**Frequency-voltage curve regions:" << std::endl;
      print_rsmi_od_volt_freq_regions(num_regions, regions);

      delete []regions;
    }
  }
}