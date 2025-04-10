#pragma once

#include <stddef.h>
int prepare_audio_data(int argc, char **argv, double **prepared_data1, double **prepared_data2, size_t *data1_len, size_t *data2_len, size_t *sample_rate);
