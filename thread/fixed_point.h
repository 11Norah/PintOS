#ifndef PINTOS_FIXED_POINT_H
#define PINTOS_FIXED_POINT_H

#define INTEGER_BITS 17
#define DECIMAL_BITS 14

typedef struct real {
    int value;
} Real;

inline Real convert_to_real(int integer)
{
    Real real_number;
    real_number.value = integer << DECIMAL_BITS;
    return real_number;
}

inline int convert_to_integer(Real real_number)
{
    return real_number.value >> DECIMAL_BITS;
}

inline int round_real_to_int(Real num)
{
    int whole = num.value / (1 << DECIMAL_BITS);
    int decimal = num.value - (whole << DECIMAL_BITS);
    if(num.value >= 0)
    {
        if (decimal >= (1 << (DECIMAL_BITS - 1))) whole = whole + 1;
    }
    else
    {
        if (decimal >= (1 << (DECIMAL_BITS - 1))) whole = whole - 1;
    }
    return whole;
}

inline Real add_real_to_int(Real real_number, int integer)
{
    Real sum;
    sum.value = real_number.value + convert_to_real(integer).value;
    return sum;
}

inline Real add_real_to_real(Real num1, Real num2)
{
    Real sum;
    sum.value = num1.value + num2.value;
    return sum;
}

inline Real subtract_int_from_real(Real real_number, int integer)
{
    Real diff;
    diff.value = real_number.value - convert_to_real(integer).value;
    return diff;
}

inline Real subtract_real_from_real(Real num1, Real num2)
{
    Real diff;
    diff.value = num1.value - num2.value;
    return diff;
}

inline Real multiply_real_and_int(Real real_number, int integer)
{
    Real prod;
    prod.value = real_number.value * integer;
    return prod;
}

inline Real multiply_real_and_real(Real num1, Real num2)
{
    Real prod;
    prod.value = (((int64_t)num1.value) * num2.value) >> DECIMAL_BITS;
    return prod;
}

inline Real divide_real_by_int(Real real_number, int integer)
{
    Real quotient;
    quotient.value = real_number.value / integer;
    return quotient;
}

inline Real divide_real_by_real(Real num1, Real num2)
{
    Real quotient;
    quotient.value = (((int64_t)num1.value) << DECIMAL_BITS) / num2.value;
    return quotient;
}

#endif //PINTOS_FIXED_POINT_H
