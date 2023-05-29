
// uint64_t waste_time(uint64_t x) {
//    x = x * 100 + 5;

//    // Calculate the sqrt of x in constant-time, then
//    // multiply by 100 and sum 5, for good measure

//    // NOTE: I know that we can start from 0x10, it's not supposed to be
//    efficient. uint64_t s = 0, b = 1llu << 32; while(b) {
//       const uint64_t t = s + b;
//       if (t * t <= x)
//          s = t;
//       b = b >> 1;
//    }

//    s = s * 100 + 5;

//    return s;
// }

// static void cache_body(register uint8_t array[],
//                               register const size_t array_size,
//                               register const size_t line_size,
//                               register const size_t num_iterations,
//                               const size_t miss_rate) ATTRIBUTE_UNOPTIMIZE;

// void cache_body(register uint8_t array[], register const size_t array_size,
//    register const size_t line_size, register const size_t num_iterations,
//    const size_t miss_rate) {
//     register const size_t increment = (line_size * miss_rate) / 100;
//     register size_t index = 0;
//     for (register size_t i = 0; i < num_iterations; ++i) {
//       array[index] = 0;
//       index += increment;
//       if (index >= array_size) {
//          index -= array_size;
//       }
//    }
// }

// #define DCACHE_MAX_SIZE     2097152
// #define DCACHE_MAX_LINESIZE 64

// #define ARRAY_SIZE (DCACHE_MAX_SIZE << 3)
// uint8_t array[ARRAY_SIZE] __attribute__((aligned(DCACHE_MAX_LINESIZE)));

// uint64_t waste_time(uint64_t xx) {
//    long num_iterations = 10L;
//    long miss_rate = 16;

//    cache_body(array, ARRAY_SIZE, DCACHE_MAX_LINESIZE, num_iterations,
//       miss_rate);

//    xx += 1;

//    return array[0] + xx;
// }

// uint64_t count_global;
// uint64_t temp_global;
