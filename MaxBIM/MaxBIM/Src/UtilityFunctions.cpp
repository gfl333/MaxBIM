#define _USE_MATH_DEFINES
#include <math.h>
#include "MaxBIM.hpp"


// degree 각도를 radian 각도로 변환
double	degreeToRad (double degree)
{
	return degree * M_PI / 180;
}

// radian 각도를 degree 각도로 변환
double	RadToDegree (double rad)
{
	return rad * 180 / M_PI;
}

// 2차원에서 2점 간의 거리를 알려줌
double	GetDistance (const double begX, const double begY, const double endX, const double endY)
{
	return sqrt ( (begX - endX) * (begX - endX) + (begY - endY) * (begY - endY) );
}

// 어떤 수가 더 큰지 비교함 : 오류(-100), A<B(-1), A==B(0), A>B(+1)
long	compareDoubles (const double a, const double b)
{
	// 같으면 0
	if ( abs (a - b) < EPS )
		return 0;

	// a > b이면 1
	if ( ((a - b) > 0) && abs (a - b) > EPS )
		return 1;

	// a < b이면 -1
	if ( ((a - b) < 0) && abs (a - b) > EPS )
		return -1;

	return -100;
}

// a와 b의 각 값 범위의 관계를 알려줌 (0~13 중 하나)
long	compareRanges (const double aMin, const double aMax, const double bMin, const double bMax)
{
	/*
	 *			 Min  Max
	 *  A   :	[10]-[20]
	 *  		 Min  Max
	 *  B
	 *	0	:	오류
	 *	1	:	[ 0]-[ 5]
	 *	2	:	[ 5]-[10]
	 *	3	:	[ 5]-[15]
	 *	4	:	[10]-[15]
	 *	5	:	[12]-[18]	IN
	 *	6	:	[15]-[20]
	 *	7	:	[15]-[25]
	 *	8	:	[20]-[25]
	 *	9	:	[25]-[30]
	 *	10	:	[ 5]-[20]
	 *	11	:	[10]-[25]
	 *	12	:	[10]-[20]
	 *	13	:	[ 5]-[25]
	 */
	
	long	result = 0;
	long	ab, aB, Ab, AB;

	// min, max 범위가 잘못되면 오류
	if ( (aMin > aMax) || (bMin > bMax) ) {
		result = 0;
		return result;
	}

	ab = compareDoubles (aMin, bMin);
	aB = compareDoubles (aMin, bMax);
	Ab = compareDoubles (aMax, bMin);
	AB = compareDoubles (aMax, bMax);

	if ( (ab  > 0) && (aB  > 0) && (Ab  > 0) && (AB  > 0) )		return 1;
	if ( (ab  > 0) && (aB == 0) && (Ab  > 0) && (AB  > 0) )		return 2;
	if ( (ab  > 0) && (aB  < 0) && (Ab  > 0) && (AB  > 0) )		return 3;
	if ( (ab == 0) && (aB  < 0) && (Ab  > 0) && (AB  > 0) )		return 4;
	if ( (ab  < 0) && (aB  < 0) && (Ab  > 0) && (AB  > 0) )		return 5;
	if ( (ab  < 0) && (aB  < 0) && (Ab  > 0) && (AB == 0) )		return 6;
	if ( (ab  < 0) && (aB  < 0) && (Ab  > 0) && (AB  < 0) )		return 7;
	if ( (ab  < 0) && (aB  < 0) && (Ab == 0) && (AB  < 0) )		return 8;
	if ( (ab  < 0) && (aB  < 0) && (Ab  < 0) && (AB  < 0) )		return 9;
	if ( (ab  > 0) && (aB  < 0) && (Ab  > 0) && (AB == 0) )		return 10;
	if ( (ab == 0) && (aB  < 0) && (Ab  > 0) && (AB  < 0) )		return 11;
	if ( (ab == 0) && (aB  < 0) && (Ab  > 0) && (AB == 0) )		return 12;
	if ( (ab  > 0) && (aB  < 0) && (Ab  > 0) && (AB  < 0) )		return 13;

	return	result;
}

// a와 b 값을 교환함
void	exchangeDoubles (double* a, double* b)
{
	double buffer;

	buffer = *a;
	*a = *b;
	*b = buffer;

	return;
}

// 시작점에서 끝점으로 향하는 벡터의 방향을 확인
long	findDirection (const double begX, const double begY, const double endX, const double endY)
{
	double deltaX = endX - begX;
	double deltaY = endY - begY;

	if ( deltaX == 0 && deltaY == 0 )	return 0;	// Point
	if ( deltaX >  0 && deltaY == 0 )	return 1;	// East-ward
	if ( deltaX >  0 && deltaY >  0 )	return 2;	// Northeast-ward
	if ( deltaX == 0 && deltaY >  0 )	return 3;	// North-ward
	if ( deltaX <  0 && deltaY >  0 )	return 4;	// Northwest-ward
	if ( deltaX <  0 && deltaY == 0 )	return 5;	// West-ward
	if ( deltaX <  0 && deltaY <  0 )	return 6;	// Southwest-ward
	if ( deltaX == 0 && deltaY <  0 )	return 7;	// South-ward
	if ( deltaX >  0 && deltaY <  0 )	return 8;	// Southeast-ward

	return -1;
}

// std::string 변수 값에 formatted string을 입력 받음
std::string	format_string (const std::string fmt, ...)
{
	int			size = ((int)fmt.size ()) * 2;
	va_list		ap;
	std::string	buffer;

	while (1) {
		buffer.resize(size);
		va_start (ap, fmt);
		int n = vsnprintf ((char*)buffer.data (), size, fmt.c_str (), ap);
		va_end (ap);

		if (n > -1 && n < size) {
			buffer.resize (n);
			return buffer;
		}

		if (n > -1)
			size = n + 1;
		else
			size *= 2;
	}

	return buffer;
}
