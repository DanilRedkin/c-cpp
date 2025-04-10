#include "performing_cross_correlation.h"

#include "return_codes.h"

#include <fftw3.h>
#include <stdio.h>
#include <string.h>

int performing_cross_correlation(double *audio_data1_in_time_domain, double *audio_data2_in_time_domain, size_t data1_size, size_t data2_size, int *result)
{
	if (!audio_data1_in_time_domain || !audio_data2_in_time_domain)
	{
		fprintf(stderr, "Invalid input parameters\n");
		return ERROR_DATA_INVALID;
	}

	int outcome = SUCCESS;

	size_t full_size = data1_size + data2_size - 1;
	size_t fft_size = full_size / 2 + 1;

	fftw_complex *all_freq_domain_data = NULL;
	fftw_complex *audio_data1_in_freq_domain = NULL;
	fftw_complex *audio_data2_in_freq_domain = NULL;
	fftw_complex *audio_data_in_freq_domain = NULL;
	double *all_time_domain_data = NULL;
	double *cross_correlated_output = NULL;
	double *audio_data1_in_time_domain_extended = NULL;
	double *audio_data2_in_time_domain_extended = NULL;
	fftw_plan fftw_plan_forward1 = NULL;
	fftw_plan fftw_plan_forward2 = NULL;
	fftw_plan fftw_plan_backward = NULL;

	all_freq_domain_data = fftw_alloc_complex(3 * fft_size);
	if (!all_freq_domain_data)
	{
		fprintf(stderr, "Failed to allocate memory for frequency domain arrays\n");
		outcome = ERROR_UNKNOWN;
		goto clean_up;
	}
	audio_data1_in_freq_domain = all_freq_domain_data;
	audio_data2_in_freq_domain = all_freq_domain_data + fft_size;
	audio_data_in_freq_domain = all_freq_domain_data + 2 * fft_size;

	all_time_domain_data = fftw_alloc_real(3 * (full_size + 1));
	if (!all_time_domain_data)
	{
		fprintf(stderr, "Failed to allocate memory for time domain arrays\n");
		outcome = ERROR_UNKNOWN;
		goto clean_up;
	}
	audio_data1_in_time_domain_extended = all_time_domain_data;
	audio_data2_in_time_domain_extended = all_time_domain_data + (full_size + 1);
	cross_correlated_output = all_time_domain_data + 2 * (full_size + 1);

	memset(audio_data1_in_time_domain_extended, 0, (full_size + 1) * sizeof(double));
	memset(audio_data2_in_time_domain_extended, 0, (full_size + 1) * sizeof(double));

	memcpy(audio_data1_in_time_domain_extended, audio_data1_in_time_domain, data1_size * sizeof(double));
	memcpy(audio_data2_in_time_domain_extended, audio_data2_in_time_domain, data2_size * sizeof(double));

	fftw_plan_forward1 =
		fftw_plan_dft_r2c_1d((int)full_size, audio_data1_in_time_domain_extended, audio_data1_in_freq_domain, FFTW_ESTIMATE);
	if (!fftw_plan_forward1)
	{
		fprintf(stderr, "Failed to create FFTW plan for audio_data1\n");
		outcome = ERROR_UNKNOWN;
		goto clean_up;
	}
	fftw_execute(fftw_plan_forward1);

	fftw_plan_forward2 =
		fftw_plan_dft_r2c_1d((int)full_size, audio_data2_in_time_domain_extended, audio_data2_in_freq_domain, FFTW_ESTIMATE);
	if (!fftw_plan_forward2)
	{
		fprintf(stderr, "Failed to create FFTW plan for audio_data2\n");
		outcome = ERROR_UNKNOWN;
		goto clean_up;
	}
	fftw_execute(fftw_plan_forward2);

	for (size_t i = 0; i < fft_size; i++)
	{
		audio_data_in_freq_domain[i][0] =
			audio_data1_in_freq_domain[i][0] * audio_data2_in_freq_domain[i][0] +
			audio_data1_in_freq_domain[i][1] * audio_data2_in_freq_domain[i][1];
		audio_data_in_freq_domain[i][1] =
			audio_data1_in_freq_domain[i][1] * audio_data2_in_freq_domain[i][0] -
			audio_data1_in_freq_domain[i][0] * audio_data2_in_freq_domain[i][1];
	}

	fftw_plan_backward = fftw_plan_dft_c2r_1d((int)full_size, audio_data_in_freq_domain, cross_correlated_output, FFTW_ESTIMATE);
	if (!fftw_plan_backward)
	{
		fprintf(stderr, "Failed to create FFTW inverse plan\n");
		outcome = ERROR_UNKNOWN;
		goto clean_up;
	}
	fftw_execute(fftw_plan_backward);

	for (size_t i = 0; i < full_size; i++)
	{
		cross_correlated_output[i] /= (double)full_size;
	}

	double pick = cross_correlated_output[0];
	size_t index = 0;
	for (size_t i = 1; i < full_size; i++)
	{
		if (cross_correlated_output[i] > pick)
		{
			pick = cross_correlated_output[i];
			index = i;
		}
	}

	*result = (2 * index > full_size) ? (int)index - (int)full_size : (int)index;

clean_up:
	if (fftw_plan_forward1)
		fftw_destroy_plan(fftw_plan_forward1);
	if (fftw_plan_forward2)
		fftw_destroy_plan(fftw_plan_forward2);
	if (fftw_plan_backward)
		fftw_destroy_plan(fftw_plan_backward);

	if (all_freq_domain_data)
		fftw_free(all_freq_domain_data);
	if (all_time_domain_data)
		fftw_free(all_time_domain_data);

	fftw_cleanup();
	return outcome;
}
