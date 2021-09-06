#include "portability.h"

#if !defined(HAVE_FFS)
    #include <stdint.h>
    /**
     * The ffs() function returns the position of the first (least significant) bit set in the word i.  The least 
     * significant bit is position 1 and the most significant position is, for example, 32 or 64.
     */
    int ffs(int v)
    {
        /* From http://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightMultLookup
         * counts the consecutive zero bits (trailing) on the right with multiply and lookup.
         * This papers explains how this works: http://citeseer.ist.psu.edu/leiserson98using.html
         */      
        static const int MultiplyDeBruijnBitPosition[32] = 
        {
            0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 
            31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
        };
        return v ? MultiplyDeBruijnBitPosition[((uint32_t)((v & -v) * 0x077CB531U)) >> 27] + 1 : 0;
    }
#endif
