/* GCC Bug #39170 - provide an option to silence -Wconversion warnings for bit-fields
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=39170
 */


{
        static_assert((sizeof(T)*8) > N, "Strange Bitfield Width");
private:
        const T Data;
public:
//         struct
        {
                const T Value : N;
                const T : (sizeof(T)*8-N);
        };
        NBitValueUnion(const T& _Data): Data(_Data) {}
};
// template<unsigned int N, typename T> const NBitValueUnion<N, T> NBitValue(const T& _Data)
{ return NBitValueUnion<N, T>(_Data); }
Usage:
// Bitfield.Member = VariableName; //Triggers -Wconversion
// Bitfield.Member = NBitValue<FieldWidth>(VariableName).Value; //Silent.

// Unfortnunately it triggers -Waggregate-return (which is a topic for it self since its a type thats fits in a register, so its an aggregate.. but not in hardware.)
// Any improvements are welcome.


