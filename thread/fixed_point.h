
#ifndef PINTOS_FIXED_POINT_H
#define PINTOS_FIXED_POINT_H
typedef struct real {
    int value;
} Real;

Real convert_to_real(int integer);
int convert_to_integer(Real real_number);
int round_real_to_int(Real num);
Real add_real_to_int(Real real_number, int integer);
Real add_real_to_real(Real num1, Real num2);
Real subtract_int_from_real(Real real_number, int integer);
Real subtract_real_from_real(Real num1, Real num2);
Real multiply_real_and_int(Real real_number, int integer);
Real multiply_real_and_real(Real num1, Real num2);
Real divide_real_by_int(Real real_number, int integer);
Real divide_real_by_real(Real num1, Real num2);

#endif //PINTOS_FIXED_POINT_H
