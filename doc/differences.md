# Differences with Redis

## String lengths, indices.

String sizes are limited to 256MB.
Indices (say in GETRANGE and SETRANGE commands) should be signed 32 bit integers in range
[-2147483647, 2147483648].

## Expiry ranges.
Expirations are limited to 4 years. For commands with millisecond precision like PEXPIRE or PSETEX,
expirations greater than 2^27ms are quietly rounded to the nearest second loosing precision of less than 0.001%.
