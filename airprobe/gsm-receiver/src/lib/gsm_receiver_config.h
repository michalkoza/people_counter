/* -*- c++ -*- */
/*
 * @file
 * @author Piotr Krysik <pkrysik@stud.elka.pw.edu.pl>
 * @section LICENSE
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 *
 * @section DESCRIPTION
 * This file contains classes which define gsm_receiver configuration
 * and the burst_counter which is used to store internal state of the receiver
 * when it's synchronized
 */
#ifndef INCLUDED_GSM_RECEIVER_CONFIG_H
#define INCLUDED_GSM_RECEIVER_CONFIG_H

#include <vector>
#include <algorithm>
#include <math.h>
#include <stdint.h>
#include <gsm_constants.h>

class multiframe_configuration
{
  private:
    multiframe_type d_type;
    std::vector<burst_type> d_burst_types;
    std::vector<bool> d_first_burst;
  public:
    multiframe_configuration() {
      d_type = unknown;
      fill(d_burst_types.begin(), d_burst_types.end(), empty);
      fill(d_first_burst.begin(), d_first_burst.end(), false);
    }

    ~multiframe_configuration() {}

    void set_type(multiframe_type type) {
      if (type == multiframe_26) {
        d_burst_types.resize(26);
        d_first_burst.resize(26);
      } else {
        d_burst_types.resize(51);
        d_first_burst.resize(51);
      }
      fill(d_burst_types.begin(), d_burst_types.end(), empty);
      fill(d_first_burst.begin(), d_first_burst.end(), false);

      d_type = type;
    }

    void set_burst_type(int nr, burst_type type) {
      d_burst_types[nr] = type;
    }

    void set_first_burst(int nr, bool first_burst) {
      d_first_burst[nr] = first_burst;
    }

    multiframe_type get_type() {
      return d_type;
    }

    burst_type get_burst_type(int nr) {
      return d_burst_types[nr];
    }

    bool get_first_burst(int nr) {
      return d_first_burst[nr];
    }
};

class burst_counter
{
  private:
    const int d_OSR;
    uint32_t d_t1, d_t2, d_t3, d_timeslot_nr;
    double d_offset_fractional;
    double d_offset_integer;
  public:
    burst_counter(int osr):
        d_OSR(osr),
        d_t1(0),
        d_t2(0),
        d_t3(0),
        d_timeslot_nr(0),
        d_offset_fractional(0.0),
        d_offset_integer(0.0) {
    }

    burst_counter(int osr, uint32_t t1, uint32_t t2, uint32_t t3, uint32_t timeslot_nr):
        d_OSR(osr),
        d_t1(t1),
        d_t2(t2),
        d_t3(t3),
        d_timeslot_nr(timeslot_nr),
        d_offset_fractional(0.0),
        d_offset_integer(0.0) {
      double first_sample_position = (get_frame_nr() * 8 + timeslot_nr) * TS_BITS;
      d_offset_integer = floor(first_sample_position);
      d_offset_fractional = first_sample_position - floor(first_sample_position);
    }

    burst_counter & operator++(int);
    void set(uint32_t t1, uint32_t t2, uint32_t t3, uint32_t timeslot_nr);

    uint32_t get_t1() {
      return d_t1;
    }

    uint32_t get_t2() {
      return d_t2;
    }

    uint32_t get_t3() {
      return d_t3;
    }

    uint32_t get_timeslot_nr() {
      return d_timeslot_nr;
    }

    uint32_t get_frame_nr() {
      return (51 * 26 * d_t1) + (51 * (((d_t3 + 26) - d_t2) % 26)) + d_t3;
    }
    
    uint32_t get_frame_nr_mod() {
      return (d_t1 << 11) + (d_t3 << 5) + d_t2;
    }

    unsigned get_offset() {
      return (unsigned)d_offset_integer;
    }
};

class channel_configuration
{
  private:
    multiframe_configuration d_timeslots_descriptions[TS_PER_FRAME];
  public:
    channel_configuration() {
      for (int i = 0; i < TS_PER_FRAME; i++) {
        d_timeslots_descriptions[i].set_type(unknown);
      }
    }

    void set_multiframe_type(int timeslot_nr, multiframe_type type) {
      d_timeslots_descriptions[timeslot_nr].set_type(type);
    }

    void set_burst_types(int timeslot_nr, const unsigned mapping[], unsigned mapping_size, burst_type b_type) {
      printf("set_burst_types_SMALLER\n");
      unsigned i;
      for (i = 0; i < mapping_size; i++) {
        d_timeslots_descriptions[timeslot_nr].set_burst_type(mapping[i], b_type);
        printf("d_timeslots_descriptions[%d][%d]=%d\n",timeslot_nr,mapping[i],b_type);
      }
    }

    void set_burst_types(int timeslot_nr, const unsigned mapping[], const unsigned first_burst[], unsigned mapping_size, burst_type b_type) {
      unsigned i;
      printf("set_burst_types_BIGGER\n");
      for (i = 0; i < mapping_size; i++) {
        d_timeslots_descriptions[timeslot_nr].set_burst_type(mapping[i], b_type);
        printf("d_timeslots_descriptions[%d][%d]=%d\t",timeslot_nr,mapping[i],b_type);        
        d_timeslots_descriptions[timeslot_nr].set_first_burst(mapping[i], first_burst[i] != 0);
        if(first_burst[i] != 0){
            printf("first = true\n");        
        }else{
            printf("first = false\n");        
        }
        
      }
    }

    void set_single_burst_type(int timeslot_nr, int burst_nr, burst_type b_type) {
      d_timeslots_descriptions[timeslot_nr].set_burst_type(burst_nr, b_type);
    }

    burst_type get_burst_type(burst_counter burst_nr);
    bool get_first_burst(burst_counter burst_nr);
};

#endif /* INCLUDED_GSM_RECEIVER_CONFIG_H */
