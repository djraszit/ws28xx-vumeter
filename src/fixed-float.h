/*
 * fixed-float.h
 *
 *      Author: nie pamiętam skąd to skopiowałem
 */

#ifndef FIXED_FLOAT_H_
#define FIXED_FLOAT_H_

/* DEFINE THE MACROS */
/* The basic operations perfomed on two numbers a and b of fixed
 point q format returning the answer in q format */
#define FADD(a,b) ((a)+(b))
#define FSUB(a,b) ((a)-(b))
#define FMUL(a,b,q) (((a)*(b))>>(q))
#define FDIV(a,b,q) (((a)<<(q))/(b))
/* The basic operations where a is of fixed point q format and b is
 an integer */
#define FADDI(a,b,q) ((a)+((b)<<(q)))
#define FSUBI(a,b,q) ((a)-((b)<<(q)))
#define FMULI(a,b) ((a)*(b))
#define FDIVI(a,b) ((a)/(b))
/* convert a from q1 format to q2 format */
#define FCONV(a, q1, q2) (((q2)>(q1)) ? (a)<<((q2)-(q1)) : (a)>>((q1)-(q2)))
/* the general operation between a in q1 format and b in q2 format
 returning the result in q3 format */
#define FADDG(a,b,q1,q2,q3) (FCONV(a,q1,q3)+FCONV(b,q2,q3))
#define FSUBG(a,b,q1,q2,q3) (FCONV(a,q1,q3)-FCONV(b,q2,q3))
#define FMULG(a,b,q1,q2,q3) FCONV((a)*(b), (q1)+(q2), q3)
#define FDIVG(a,b,q1,q2,q3) (FCONV(a, q1, (q2)+(q3))/(b))
/* convert to and from floating point */

#define TOFIX(d, q) ((int64_t)( (d)*(double)(1<<(q)) ))
#define TOFLT(a, q) ( (double)(a) / (double)(1<<(q)) )
#define TEST(FIX, FLT, STR) { \
a = a1 = randint(); \
b = bi = a2 = randint(); \
fa = TOFLT(a, q); \
fa1 = TOFLT(a1, q1); \
fa2 = TOFLT(a2, q2); \
fb = TOFLT(b, q); \
ans1 = FIX; \
ans2 = FLT; \
printf("Testing %s\n fixed point answer=%f\n floating point answer=%f\n", \
STR, TOFLT(ans1, q), ans2); \
}

#endif /* FIXED_FLOAT_H_ */
