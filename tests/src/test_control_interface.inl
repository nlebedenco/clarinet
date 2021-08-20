static void assert_can_initialize(int n)
{
    static const clarinet_errcode expected = CLARINET_ENONE;
    while (n--)
    {
        const clarinet_errcode actual = clarinet_initialize();
        assert_int_equal(expected, actual);
    }
}

static void assert_can_finalize(int n)
{
    static const clarinet_errcode expected = CLARINET_ENONE;

    while (n--)
    {
        const clarinet_errcode actual = clarinet_finalize();
        assert_int_equal(expected, actual);
    }
}

