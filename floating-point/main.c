#include "return_codes.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#define UINT32_LENGTH 32
#define UINT64_LENGTH 64

typedef enum
{
	Number,
	Zero,
	Infinity,
	NaN
} spec_type;

typedef struct
{
	int16_t exp_min;
	int16_t exp_max;
	int16_t exp_min_denorm;
	uint8_t size;
	uint8_t mnt_len;
	uint8_t exp_len;
} Constants;

typedef struct
{
	uint32_t mnt;
	int16_t exp;
	uint8_t sign;
	spec_type ft_spec_t;
} FloatType;

void init_consts_f(Constants *consts)
{
	consts->size = 32;
	consts->mnt_len = 23;
	consts->exp_len = 8;
	consts->exp_min = -127;
	consts->exp_max = 128;
	consts->exp_min_denorm = -149;
}

void init_consts_h(Constants *consts)
{
	consts->size = 16;
	consts->mnt_len = 10;
	consts->exp_len = 5;
	consts->exp_min = -15;
	consts->exp_max = 16;
	consts->exp_min_denorm = -24;
}

uint32_t cut_bits_32(uint32_t bits, uint8_t start, uint8_t end)
{
	return ((bits & (((uint64_t)1 << (UINT32_LENGTH - start + 1)) - 1)) >> (UINT32_LENGTH - end));
}
uint64_t make_ones(uint8_t length)
{
	return (((uint64_t)1 << length) - 1);
}
uint8_t get_bit(uint64_t num, uint8_t place)
{
	return (num >> (place - 1)) & 1;
}
uint64_t get_n_bits(uint64_t num, uint8_t length)
{
	return num & make_ones(length);
}

uint8_t bit_length(uint64_t number)
{
	uint8_t length = 0;
	while (number > 0)
	{
		length++;
		number >>= 1;
	}
	return length;
}

uint64_t add_bit(uint64_t num, uint8_t place)
{
	return num + ((uint64_t)1 << (place - 1));
}
uint64_t subtract_bit(uint64_t num, uint8_t place)
{
	return num - ((uint64_t)1 << (place - 1));
}

void init_float(FloatType *ft, uint32_t num, const Constants *consts)
{
	int prefix = UINT32_LENGTH - consts->size;
	ft->sign = num >> (consts->size - 1);
	uint32_t exponent_bits = cut_bits_32(num, prefix + 2, prefix + 1 + consts->exp_len);
	ft->exp = exponent_bits + consts->exp_min;
	ft->mnt = cut_bits_32(num, prefix + 2 + consts->exp_len, UINT32_LENGTH);
	ft->ft_spec_t = Number;
	if (ft->mnt == 0)
	{
		if (ft->exp == consts->exp_min)
			ft->ft_spec_t = Zero;
		else if (ft->exp == consts->exp_max)
			ft->ft_spec_t = Infinity;
	}
	else if (ft->exp == consts->exp_max)
		ft->ft_spec_t = NaN;
}

void normalize(FloatType *ft, const Constants *consts)
{
	if (ft->ft_spec_t != Number || ft->exp != consts->exp_min)
		return;
	ft->exp++;
	uint8_t zero_num = consts->mnt_len - bit_length(ft->mnt);
	uint8_t offset = zero_num + 1;
	ft->exp -= offset;
	ft->mnt = (ft->mnt << offset) - ((uint32_t)1 << consts->mnt_len);
}
void print_float(const FloatType *ft, const Constants *consts)
{
	uint8_t offset = (4 - (consts->mnt_len % 4));
	uint8_t char_count = (consts->mnt_len + offset) / 4;
	uint32_t mnt_print = ft->mnt << offset;
	char *sign = (ft->sign == 1) ? "-" : "";
	char *exp_sign = (ft->exp < 0) ? "" : "+";
	switch (ft->ft_spec_t)
	{
	case Number:
		printf("%s0x1.%0*" PRIx32 "p%s%" PRId16 "\n", sign, char_count, mnt_print, exp_sign, ft->exp);
		break;
	case Zero:
		printf("%s0x0.%0*xp+0\n", sign, consts->exp_len - 2, 0);
		break;
	case Infinity:
		printf("%sinf", sign);
		break;
	case NaN:
		printf("nan");
		break;
	}
}

int read_args(int argc, char **argv, char *fmt, uint8_t *rounding, uint32_t *num1, char *op_sign, uint32_t *num2)
{
	int read_flag;
	if (argc != 4 && argc != 6)
	{
		fprintf(stderr, "Wrong number of arguments\n");
		return ERROR_ARGUMENTS_INVALID;
	}
	if (argv[1][1] != '\0')
	{
		fprintf(stderr, "Format must be character\n");
		return ERROR_ARGUMENTS_INVALID;
	}
	*fmt = argv[1][0];
	read_flag = sscanf(argv[2], "%" SCNu8, rounding);
	if (read_flag != 1 || *rounding > 3)
	{
		fprintf(stderr, "Rounding type must be in {0,1,2,3}\n");
		return ERROR_ARGUMENTS_INVALID;
	}
	read_flag = sscanf(argv[3], "%" SCNx32, num1);
	if (read_flag != 1)
	{
		fprintf(stderr, "First number is incorrect\n");
		return ERROR_ARGUMENTS_INVALID;
	}
	if (argc == 6)
	{
		if (argv[4][1] != '\0')
		{
			fprintf(stderr, "Operation sign must be character\n");
			return ERROR_ARGUMENTS_INVALID;
		}
		*op_sign = argv[4][0];
		read_flag = sscanf(argv[5], "%" SCNx32, num2);
		if (read_flag != 1)
		{
			fprintf(stderr, "Second number is incorrect\n");
			return ERROR_ARGUMENTS_INVALID;
		}
	}
	return SUCCESS;
}

void rounding(FloatType *result, uint64_t offset, uint8_t offset_len, uint8_t zero_num, uint8_t round_num, const Constants *consts)
{
	if (result->exp < consts->exp_min - consts->mnt_len + 1)
	{
		result->ft_spec_t = Zero;
	}
	if (result->exp >= consts->exp_max)
	{
		result->ft_spec_t = Infinity;
	}
	if (result->ft_spec_t != Infinity && result->ft_spec_t != Zero && offset == 0)
		return;
	switch (round_num)
	{
	case 0:
		break;
	case 1:
		if (get_bit(offset, offset_len) == 0)
			return;
		if (get_n_bits(offset, offset_len - 1) > 0)
			result->mnt = add_bit(result->mnt, zero_num + 1);
		if (get_n_bits(offset, offset_len - 1) == 0)
			result->mnt += get_bit(result->mnt, zero_num + 1);
		break;
	case 2:
		if (result->sign == 0)
			result->mnt = add_bit(result->mnt, zero_num + 1);
		break;
	case 3:
		if (result->sign == 1)
			result->mnt = add_bit(result->mnt, zero_num + 1);
		break;
	}
	if (get_bit(result->mnt, consts->mnt_len + 1))
	{
		result->exp++;
		result->mnt = 0;
	}
}
void add(FloatType *ft1, FloatType *ft2, uint8_t round_num, FloatType *result, const Constants *consts)
{
	int spec_flag = 1;
	if (ft1->ft_spec_t == NaN || ft2->ft_spec_t == NaN ||
		(ft1->ft_spec_t == Infinity && ft2->ft_spec_t == Infinity && ft1->sign != ft2->sign))
		result->ft_spec_t = NaN;
	else if (ft1->ft_spec_t == Infinity)
		*result = *ft1;
	else if (ft2->ft_spec_t == Infinity)
		*result = *ft2;
	else if (ft1->ft_spec_t == Zero && ft2->ft_spec_t == Zero && ft2->sign == 1)
		*result = *ft1;
	else if (ft1->ft_spec_t == Zero && ft2->ft_spec_t == Zero)
		*result = *ft2;
	else if (ft1->ft_spec_t == Zero)
		*result = *ft2;
	else if (ft2->ft_spec_t == Zero)
		*result = *ft1;
	else
	{
		result->ft_spec_t = Number;
		spec_flag = 0;
	}
	if (spec_flag)
		return;

	if ((ft2->exp > ft1->exp) || (ft2->exp == ft1->exp && ft2->mnt > ft1->mnt) ||
		(ft2->exp == ft1->exp && ft2->mnt == ft1->mnt && ft1->sign == 1))
	{
		FloatType ft_temp = *ft1;
		*ft1 = *ft2;
		*ft2 = ft_temp;
	}

	result->exp = ft1->exp;
	result->sign = ft1->sign;
	uint8_t unit_place = consts->mnt_len + 1;
	uint64_t mnt_1 = add_bit(ft1->mnt, unit_place);
	uint64_t mnt_2 = add_bit(ft2->mnt, unit_place);
	uint16_t exp_dif = ft1->exp - ft2->exp;
	uint64_t mnt_add;
	uint64_t offset = 0;
	uint8_t offset_len = 0;
	uint8_t zero_num = 0;

	if (exp_dif > unit_place + 2)
	{
		mnt_add = mnt_1;
		offset_len = 4;
		offset = 1;
		if (ft1->sign != ft2->sign)
		{
			mnt_add <<= offset_len;
			mnt_add -= 1;
			uint8_t lost_bit = (bit_length(mnt_add) != unit_place + offset_len);
			result->exp -= lost_bit;
			offset_len -= lost_bit;
			offset = get_n_bits(mnt_add, offset_len);
			mnt_add >>= offset_len;
		}
		result->mnt = subtract_bit(mnt_add, unit_place);
		rounding(result, offset, offset_len, 0, round_num, consts);
		return;
	}

	mnt_1 <<= exp_dif;
	if (ft1->sign == ft2->sign)
	{
		mnt_add = mnt_1 + mnt_2;
		uint8_t overflow_bit = get_bit(mnt_add, unit_place + exp_dif + 1);
		offset_len = exp_dif;
		if (overflow_bit)
		{
			offset_len++;
			result->exp++;
		}
	}
	else
	{
		mnt_add = mnt_1 - mnt_2;
		if (mnt_add == 0)
		{
			result->ft_spec_t = Zero;
			return;
		}
		uint8_t mnt_add_len = bit_length(mnt_add);
		uint8_t mnt_dif = (unit_place + exp_dif) - mnt_add_len;
		result->exp -= mnt_dif;
		if (mnt_add_len > unit_place)
			offset_len = mnt_add_len - unit_place;
		else
			mnt_add <<= (unit_place - mnt_add_len);
	}

	if (result->exp <= consts->exp_min)
		zero_num = consts->exp_min - result->exp + 1;
	offset_len += zero_num;
	offset = get_n_bits(mnt_add, offset_len);
	mnt_add >>= offset_len;
	mnt_add <<= zero_num;
	result->mnt = subtract_bit(mnt_add, unit_place);
	rounding(result, offset, offset_len, zero_num, round_num, consts);
}
void divide(FloatType *ft1, FloatType *ft2, uint8_t round_num, FloatType *result, const Constants *consts)
{
	result->sign = ft1->sign ^ ft2->sign;
	if (ft1->ft_spec_t == NaN || ft2->ft_spec_t == NaN || (ft1->ft_spec_t == Zero && ft2->ft_spec_t == Zero) ||
		(ft1->ft_spec_t == Infinity && ft2->ft_spec_t == Infinity))
		result->ft_spec_t = NaN;
	else if (ft1->ft_spec_t == Infinity || ft2->ft_spec_t == Zero)
		result->ft_spec_t = Infinity;
	else if (ft1->ft_spec_t == Zero || ft2->ft_spec_t == Infinity)
		result->ft_spec_t = Zero;
	else
		result->ft_spec_t = Number;
	if (result->ft_spec_t != Number)
		return;

	result->exp = ft1->exp - ft2->exp;
	uint8_t unit_place = consts->mnt_len + 1;
	uint8_t division_offset = UINT64_LENGTH - unit_place;
	uint64_t mnt_1 = add_bit(ft1->mnt, unit_place);
	uint64_t mnt_2 = add_bit(ft2->mnt, unit_place);
	mnt_1 <<= division_offset;
	uint64_t mnt_div = mnt_1 / mnt_2;
	uint64_t mnt_div_len = bit_length(mnt_div);
	if (mnt_div_len == division_offset)
		result->exp--;
	uint8_t zero_num = 0;
	if (result->exp <= consts->exp_min)
		zero_num = consts->exp_min - result->exp + 1;
	uint8_t offset_len = mnt_div_len - unit_place + zero_num;
	uint64_t offset = get_n_bits(mnt_div, offset_len);
	mnt_div >>= offset_len;
	mnt_div <<= zero_num;
	result->mnt = subtract_bit(mnt_div, unit_place);
	if ((mnt_1 % mnt_2) != 0)
	{
		offset = (offset << 1) + 1;
		offset_len++;
	}
	rounding(result, offset, offset_len, zero_num, round_num, consts);
}
void multiply(FloatType *ft1, FloatType *ft2, uint8_t round_num, FloatType *result, const Constants *consts)
{
	result->sign = ft1->sign ^ ft2->sign;
	if (ft1->ft_spec_t == NaN || ft2->ft_spec_t == NaN || (ft1->ft_spec_t == Infinity && ft2->ft_spec_t == Zero) ||
		(ft1->ft_spec_t == Zero && ft2->ft_spec_t == Infinity))
		result->ft_spec_t = NaN;
	else if (ft1->ft_spec_t == Infinity || ft2->ft_spec_t == Infinity)
		result->ft_spec_t = Infinity;
	else if (ft1->ft_spec_t == Zero || ft2->ft_spec_t == Zero)
		result->ft_spec_t = Zero;
	else
		result->ft_spec_t = Number;
	if (result->ft_spec_t != Number)
		return;

	result->exp = ft1->exp + ft2->exp;
	uint8_t unit_place = consts->mnt_len + 1;
	uint64_t mnt_with_bit = add_bit(ft1->mnt, unit_place) * add_bit(ft2->mnt, unit_place);
	uint8_t offset_len = consts->mnt_len;
	if (bit_length(mnt_with_bit) == unit_place * 2)
	{
		offset_len += 1;
		result->exp += 1;
	}
	uint8_t zero_num = 0;
	if (result->exp <= consts->exp_min)
		zero_num = consts->exp_min - result->exp + 1;
	offset_len += zero_num;
	uint64_t offset = get_n_bits(mnt_with_bit, offset_len);
	mnt_with_bit >>= offset_len;
	mnt_with_bit <<= zero_num;
	result->mnt = subtract_bit(mnt_with_bit, unit_place);
	rounding(result, offset, offset_len, zero_num, round_num, consts);
}
int evaluate_operation(FloatType *ft1, char op_sign, FloatType *ft2, uint8_t rounding, FloatType *result, const Constants *consts)
{
	switch (op_sign)
	{
	case '+':
		add(ft1, ft2, rounding, result, consts);
		break;
	case '-':
		ft2->sign = 1 - ft2->sign;
		add(ft1, ft2, rounding, result, consts);
		break;
	case '*':
		multiply(ft1, ft2, rounding, result, consts);
		break;
	case '/':
		divide(ft1, ft2, rounding, result, consts);
		break;
	default:
		fprintf(stderr, "Unknown operation\n");
		return ERROR_ARGUMENTS_INVALID;
	}
	return SUCCESS;
}
int main(int argc, char *argv[])
{
	char fmt, op_sign;
	uint8_t round_num;
	uint32_t num1, num2;
	int return_value = 0;
	if ((return_value = read_args(argc, argv, &fmt, &round_num, &num1, &op_sign, &num2)) != 0)
		return return_value;

	Constants consts;
	switch (fmt)
	{
	case 'h':
		init_consts_h(&consts);
		break;
	case 'f':
		init_consts_f(&consts);
		break;
	default:
		fprintf(stderr, "Format must be 'h'/'f'\n");
		return ERROR_ARGUMENTS_INVALID;
	}
	FloatType ft1;
	init_float(&ft1, num1, &consts);
	normalize(&ft1, &consts);

	if (argc == 6)
	{
		FloatType ft2, result;
		init_float(&ft2, num2, &consts);
		normalize(&ft2, &consts);
		return_value = evaluate_operation(&ft1, op_sign, &ft2, round_num, &result, &consts);
		if (return_value != 0)
			return return_value;
		print_float(&result, &consts);
	}
	else
		print_float(&ft1, &consts);
	return SUCCESS;
}
