/****************************************************************************
 *
 *   Copyright 2019 NXP.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file hello_example.cpp
 * Race code for NXP Cup
 *
 * @author Katrin Moritz
 */

#include "nxpcup_race.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

uint8_t get_num_vectors(Vector &vec1, Vector &vec2) {
	uint8_t numVectors = 0;
	if(!(vec1.m_x0 == 0 && vec1.m_x1 == 0 && vec1.m_y0 == 0 && vec1.m_y1 == 0)) numVectors++;
	if(!(vec2.m_x0 == 0 && vec2.m_x1 == 0 && vec2.m_y0 == 0 && vec2.m_y1 == 0)) numVectors++;
	return numVectors;
}

Vector copy_vectors(pixy_vector_s &pixy, uint8_t num) {
	Vector vec;
	if(num == 1) {
		vec.m_x0 = pixy.m0_x0;
		vec.m_x1 = pixy.m0_x1;
		vec.m_y0 = pixy.m0_y0;
		vec.m_y1 = pixy.m0_y1;
	}
	if(num == 2) {
		vec.m_x0 = pixy.m1_x0;
		vec.m_x1 = pixy.m1_x1;
		vec.m_y0 = pixy.m1_y0;
		vec.m_y1 = pixy.m1_y1;
	}
	return vec;
}

roverControl raceTrack(pixy_vector_s &pixy)
{
	Vector main_vec = copy_vectors(pixy, 1);
	Vector vec1 = main_vec;
	Vector vec2 = copy_vectors(pixy, 2);
	_vector vec_first;				// vector from two point of vec1
	_vector vec_second;				// vector from two point of vec2
	float angle;					// angle between vec_first and vec_second
	uint8_t frameWidth = 79;
	uint8_t frameHeight = 52;
	int16_t window_center = (frameWidth / 2);
	roverControl control{};
	float x, y;					 // calc gradient and position of main vector
	static hrt_abstime no_line_time = 0;		// time variable for time since no line detected
	hrt_abstime time_diff = 0;
	static bool first_call = true;
	uint8_t num_vectors = get_num_vectors(vec1, vec2);


	switch (num_vectors) {
	case 0:
		if(first_call){
			no_line_time = hrt_absolute_time();
			first_call = false;
		}else{
			time_diff = hrt_elapsed_time_atomic(&no_line_time);
			control.steer = 0.0f;
			if(time_diff > 1000000){
				/* Stopping if no vector is available */
				control.steer = 0.0f;
				control.speed = SPEED_STOP;
			}
		}
		break;

	case 2:
		first_call = true;
		// Calc of vectors from two points
		if (vec1.m_x1 > vec1.m_x0) {
			vec_first.x = (float)(vec1.m_x1 - vec1.m_x0) / (float)frameWidth;
			vec_first.y = (float)(vec1.m_y1 - vec1.m_y0) / (float)frameHeight;

		} else {
			vec_first.x = (float)(vec1.m_x0 - vec1.m_x1) / (float)frameWidth;
			vec_first.y = (float)(vec1.m_y0 - vec1.m_y1) / (float)frameHeight;
		}

		if (vec2.m_x1 > vec2.m_x0) {
			vec_second.x = (float)(vec2.m_x1 - vec2.m_x0) / (float)frameWidth;
			vec_second.y = (float)(vec2.m_y1 - vec2.m_y0) / (float)frameHeight;

		} else {
			vec_second.x = (float)(vec2.m_x0 - vec2.m_x1) / (float)frameWidth;
			vec_second.y = (float)(vec2.m_y0 - vec2.m_y1) / (float)frameHeight;
		}

		// Calc the angle between the two vectors
		vec_first.norm = sqrt(vec_first.x * vec_first.x + vec_first.y * vec_first.y);
		vec_second.norm = sqrt(vec_second.x * vec_second.x + vec_second.y * vec_second.y);
		angle = (vec_first.x * vec_second.x + vec_first.y * vec_second.y) / (vec_first.norm * vec_second.norm);

		if (angle < 0.9f) {
			//Calc the middle of both vector coordinates m_x1
			main_vec.m_x1 = (vec1.m_x1 + vec1.m_x1) / 2;

			// Calculate heading error with respect to m_x1, which is the far-end of the vector
			control.steer = (float)(main_vec.m_x1 - window_center) / (float)frameWidth;

			control.speed = SPEED_FAST;
			break;
		}else if (angle > 1.4f){ // lines have a right angle --> possible Intersection
			control.speed = SPEED_SLOW;
			control.steer = 0.0f;
		}

	default:
		first_call = true;
		/* Following the main vector */
		if (vec1.m_x1 > vec1.m_x0) {
			x = (float)(vec1.m_x1 - vec1.m_x0) / (float)frameWidth;
			y = (float)(vec1.m_y1 - vec1.m_y0) / (float)frameHeight;
		} else {
			x = (float)(vec1.m_x0 - vec1.m_x1) / (float)frameWidth;
			y = (float)(vec1.m_y0 - vec1.m_y1) / (float)frameHeight;
		}
		if(vec1.m_x0 != vec1.m_x1){
			control.steer = (-1) * x / y; // Gradient of the main vector
			control.speed = SPEED_NORMAL;
		}else{
			control.steer = 0.0;
			control.speed = SPEED_SLOW;
		}
		break;
	}

	return control;
}
