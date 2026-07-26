/* Per the C standard's "pp-number" grammar, a digit followed by any run
 * of digits/identifier-characters/'.' forms a SINGLE preprocessing token
 * — this is deliberately looser than a real numeric-literal token,
 * specifically so something like "10baseT_Half" survives macro-argument
 * substitution as one coherent token, ready for later "##" token-pasting
 * to build an identifier like "FOO_10baseT_Half_BIT" out of it.
 *
 * rcc's lexer already implemented the pp-number suffix-consuming loop
 * correctly OUTSIDE of directives, but gated the *entire* loop (not just
 * its "invalid numeric suffix" diagnostic) behind "not currently lexing a
 * directive/macro-definition line". A #define BODY containing
 * "10baseT_Half" as plain text (e.g. as a macro argument, not itself the
 * "##" operand) is lexed while lex_in_directive is true — so the number
 * lexer stopped right after "10", leaving "baseT_Half" as a separate
 * identifier token, before ## pasting ever got a chance to see the whole
 * thing as one fragment. The two resulting paste operations then built
 * two unrelated, meaningless identifiers instead of one real symbol.
 *
 * Found via a real Linux kernel build: include/uapi/linux/ethtool.h's
 *   #define __ETHTOOL_LINK_MODE_LEGACY_MASK(base_name) \
 *       (1UL << (ETHTOOL_LINK_MODE_ ## base_name ## _BIT))
 * combined with (include/linux/mii.h, mdio.h use dozens of these)
 *   #define ADVERTISED_10baseT_Half __ETHTOOL_LINK_MODE_LEGACY_MASK(10baseT_Half)
 * — "10baseT_Half" sits inside a #define body, so it split into "10" and
 * "baseT_Half" before pasting, producing "ETHTOOL_LINK_MODE_10" and
 * "baseT_Half_BIT" instead of the real enum constant
 * "ETHTOOL_LINK_MODE_10baseT_Half_BIT", reported as a cascade of
 * "undeclared variable" errors across net/ethernet/eth.o and everything
 * else pulling in mii.h/mdio.h.
 */
enum {
    MYENUM_10baseT_Half_BIT = 3,
    MYENUM_100baseT_Full_BIT = 7,
};

#define __LEGACY_MASK(base_name) (1UL << (MYENUM_ ## base_name ## _BIT))

/* Mirrors ADVERTISED_10baseT_Half: the pasted argument lives inside this
 * #define's own body, not at the direct call site in "normal" code. */
#define MY_10baseT_Half __LEGACY_MASK(10baseT_Half)
#define MY_100baseT_Full __LEGACY_MASK(100baseT_Full)

int main(void)
{
    if (MY_10baseT_Half != (1UL << MYENUM_10baseT_Half_BIT)) return 1;
    if (MY_100baseT_Full != (1UL << MYENUM_100baseT_Full_BIT)) return 2;
    return 0;
}
