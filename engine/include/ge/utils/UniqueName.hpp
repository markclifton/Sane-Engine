#pragma once

#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b
#define UNIQUE_NAME_(base) CONCAT(base, __COUNTER__)
#define PRIVATE_NAME(name) CONCAT(CONCAT(__, UNIQUE_NAME_(name)), __)