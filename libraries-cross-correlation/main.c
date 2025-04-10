#include "audio_data_preparation.h"
#include "performing_cross_correlation.h"
#include "return_codes.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	if (argc < 2 || argc > 3)
	{
		fprintf(stderr, "Incorrect number of files provided\n");
		return ERROR_ARGUMENTS_INVALID;
	}

	double *prepared_data1 = NULL;
	double *prepared_data2 = NULL;
	size_t data1_len = 0;
	size_t data2_len = 0;
	size_t sample_rate = 0;

	int result = prepare_audio_data(argc, argv, &prepared_data1, &prepared_data2, &data1_len, &data2_len, &sample_rate);
	if (result != SUCCESS)
	{
		fprintf(stderr, "Error preparing audio data\n");
		goto clean_up;
	}

	int samples_delta = 0;
	result = performing_cross_correlation(prepared_data1, prepared_data2, data1_len, data2_len, &samples_delta);
	if (result != SUCCESS)
	{
		fprintf(stderr, "Error calculating cross-correlation\n");
		goto clean_up;
	}

	printf("delta: %i samples\nsample rate: %i Hz\ndelta time: %i ms\n",
		   (int)samples_delta,
		   (int)sample_rate,
		   (int)((double)samples_delta * 1000 / (double)sample_rate));

clean_up:
	if (prepared_data1)
	{
		free(prepared_data1);
		prepared_data1 = NULL;
	}
	if (prepared_data2)
	{
		free(prepared_data2);
		prepared_data2 = NULL;
	}
	return result;
}
