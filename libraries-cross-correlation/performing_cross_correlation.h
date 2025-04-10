#pragma once

#include <stddef.h>
int performing_cross_correlation(double *audio_data1_in_time_domain, double *audio_data2_in_time_domain, size_t data1_size, size_t data2_size, int *index);
