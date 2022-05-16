#include "fixed_point.h"

#define INTEGER_BITS 17
#define DECIMAL_BITS 14

Real convert_to_real(int integer)
{
    Real real_number;
    real_number.value = integer << DECIMAL_BITS;
    return real_number;
}

int convert_to_integer(Real real_number)
{
    return real_number.value >> DECIMAL_BITS;
}

int round_real_to_int(Real num)
{
    int whole = num.value / (1 << DECIMAL_BITS);
    int decimal = num.value - (whole << DECIMAL_BITS);
    if(decimal >= (1 << (DECIMAL_BITS-1))) return whole + 1;
    else return whole;
}

Real add_real_to_int(Real real_number, int integer)
{
    Real sum;
    sum.value = real_number.value + convert_to_real(integer);
    return sum;
}

Real add_real_to_real(Real num1, Real num2)
{
    Real sum;
    sum.value = num1.value + num2.value;
    return sum;
}

Real subtract_int_from_real(Real real_number, int integer)
{
    Real diff;
    diff.value = real_number.value - convert_to_real(integer);
    return diff;
}

Real subtract_real_from_real(Real num1, Real num2)
{
    Real diff;
    diff.value = num1.value - num2.value;
    return diff;
}

Real multiply_real_and_int(Real real_number, int integer)
{
    Real prod;
    prod.value = real_number.value * integer;
    return prod;
}

Real multiply_real_and_real(Real num1, Real num2)
{
    Real prod;
    prod.value = (num1.value * num2.value) >> DECIMAL_BITS;
    return prod;
}

Real divide_real_by_int(Real real_number, int integer)
{
    Real quotient;
    quotient.value = real_number.value / integer;
    return quotient;
}

Real divide_real_by_real(Real num1, Real num2)
{
    Real quotient;
    quotient.value = (num1.value << DECIMAL_BITS) / num2.value;
    return quotient;
}









