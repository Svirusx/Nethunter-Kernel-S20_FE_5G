
/*
* Copyright (c) 2020, STMicroelectronics - All Rights Reserved
*
* This file is part of VL53L5 Kernel Driver and is dual licensed,
* either 'STMicroelectronics Proprietary license'
* or 'BSD 3-clause "New" or "Revised" License' , at your option.
*
********************************************************************************
*
* 'STMicroelectronics Proprietary license'
*
********************************************************************************
*
* License terms: STMicroelectronics Proprietary in accordance with licensing
* terms at www.st.com/sla0081
*
* STMicroelectronics confidential
* Reproduction and Communication of this document is strictly prohibited unless
* specifically authorized in writing by STMicroelectronics.
*
*
********************************************************************************
*
* Alternatively, VL53L5 Kernel Driver may be distributed under the terms of
* 'BSD 3-clause "New" or "Revised" License', in which case the following
* provisions apply instead of the ones mentioned above :
*
********************************************************************************
*
* License terms: BSD 3-clause "New" or "Revised" License.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its contributors
* may be used to endorse or promote products derived from this software
* without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*
********************************************************************************
*
*/

#include "vl53l5_dci_utils.h"

uint8_t vl53l5_decode_uint8_t(
	uint16_t count,
	uint8_t *pbuffer)
{
	(void)count;

	return (uint8_t)(*pbuffer);
}

void vl53l5_encode_uint8_t(
	uint8_t ip_value,
	uint16_t count,
	uint8_t *pbuffer)
{
	(void)count;

	*pbuffer = (uint8_t)(ip_value & 0x00FF);
}

int8_t vl53l5_decode_int8_t(
	uint16_t count,
	uint8_t *pbuffer)
{
	(void)count;

	return (int8_t)(*pbuffer);
}

void vl53l5_encode_int8_t(
	int8_t ip_value,
	uint16_t count,
	uint8_t *pbuffer)
{
	(void)count;

	*pbuffer = (uint8_t)(ip_value & 0x00FF);
}

uint16_t vl53l5_decode_uint16_t(
	uint16_t count,
	uint8_t *pbuffer)
{

	uint16_t value = 0x00;

	while (count-- > 0)
		value = (value << 8) | (uint16_t)pbuffer[count];

	return value;
}

void vl53l5_encode_uint16_t(
	uint16_t ip_value,
	uint16_t count,
	uint8_t *pbuffer)
{

	uint16_t i = 0;
	uint16_t data = 0;

	data = ip_value;

	for (i = 0; i < count ; i++) {
		pbuffer[i] = (uint8_t)(data & 0x00FF);
		data = data >> 8;
	}
}

void vl53l5_encode_int16_t(
	int16_t ip_value,
	uint16_t count,
	uint8_t *pbuffer)
{

	uint16_t i = 0;
	int16_t data = 0;

	data = ip_value;

	for (i = 0; i < count; i++) {
		pbuffer[i] = (uint8_t)(data & 0xFF);
		data = data >> 8;
	}
}

int16_t vl53l5_decode_int16_t(
	uint16_t count,
	uint8_t *pbuffer)
{
	int16_t value = 0x00;

	if (pbuffer[count - 1] & 0x80)
		value = (int16_t)0xFFFF;

	while (count-- > 0)
		value = (value << 8) | (int16_t)pbuffer[count];

	return value;
}

void vl53l5_encode_int32_t(
	int32_t ip_value,
	uint16_t count,
	uint8_t *pbuffer)
{

	uint16_t i = 0;
	int32_t data = 0;

	data = ip_value;

	for (i = 0; i < count ; i++) {
		pbuffer[i] = (uint8_t)(data & 0x00FF);
		data = data >> 8;
	}
}

int32_t vl53l5_decode_int32_t(
	uint16_t count,
	uint8_t *pbuffer)
{
	int32_t value = 0x00;

	if (pbuffer[count - 1] & 0x80)
		value = 0xFFFF;

	while (count-- > 0)
		value = (value << 8) | (int32_t)pbuffer[count];

	return value;
}

void vl53l5_encode_uint32_t(
	uint32_t ip_value,
	uint16_t count,
	uint8_t *pbuffer)
{

	uint16_t i = 0;
	uint32_t data = 0;

	data = ip_value;

	for (i = 0; i < count ; i++) {
		pbuffer[i] = (uint8_t)(data & 0x00FF);
		data = data >> 8;
	}
}

uint32_t vl53l5_decode_uint32_t(
	uint16_t count,
	uint8_t *pbuffer)
{
	uint32_t value = 0x00;

	while (count-- > 0)
		value = (value << 8) | (uint32_t)pbuffer[count];

	return value;
}
