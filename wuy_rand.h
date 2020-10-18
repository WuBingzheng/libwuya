#ifndef WUY_RAND_H
#define WUY_RAND_H

static inline long wuy_rand_range(long range)
{
	return random() / (RAND_MAX / range + 1);
}

static inline double wuy_rand_double(void)
{
	return (double)random() / ((double)RAND_MAX + 1);
}

static inline bool wuy_rand_sample(double rate)
{
	return random() < RAND_MAX * rate;
}

#endif
