/* GCC Bug #88895 - unnecessary -Wshift-count-overflow in unreachable code (shift)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88895
 */


volatile unsigned int q = 56; //now the compiler can't assume the 
// shift is a problem
//      (T)v << q;
// or
//      (T)v * (((uintmax_t)1 << 56))
// but there goes hope of optimization, too. And for what I'm doing 
// optimization is essential.
// I'm not looking for ways to code work around a warning. I'm just asking 
for a bugfix in the compiler.
// On 1/17/19 7:38 PM, msebor at gcc dot gnu.org wrote:
// <span class="quote">> <a href="https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88895">https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88895</a></span >
// >
// <span class="quote">> --- <a href="show_bug.cgi?id=88895#c6">Comment #6</a> from Martin Sebor <msebor at gcc dot gnu.org> ---
// > The C++ solution to this kind of problem is to use template specialization.
// > It's a been a while since I wrote any real C++ code but this seems to work and
// > avoids the warnings</span >
// >
// <span class="quote">> class PackRule
> {
// >    template <class T, unsigned N>
// >    struct Packer;
// > public:
// >    const unsigned short offset_;
// >    const unsigned char width_;</span >
// >
// <span class="quote">>    inline PackRule(const unsigned short offset, const unsigned char width) :
>      offset_(offset), width_(width) {}</span >
// >
// <span class="quote">>    template <typename T>
// >    inline void pack (void* to, const T v) const
>    {
// >      unsigned char* tobyte = (unsigned char*)to + offset_;
// >      Packer<T, sizeof (T)>::pack (tobyte, v);
>    }</span >
// >
// <span class="quote">>    template<typename T>
// >    inline T unpack (const void* from, T* output) const
>    {
// >      const unsigned char* frombyte = (const unsigned char*)from + offset_;
// >      return *output = Packer<T, sizeof (T)>::unpack (frombyte);
>    }
> };</span >
// >
// <span class="quote">> template <class T, unsigned N>
// > struct PackRule::Packer
> {
// >    static void pack (unsigned char *p, T x)
>    {
// >      p[sizeof (T) - N] = (unsigned char)(x >> (N - 1) * __CHAR_BIT__);
// >      Packer<T, N - 1>::pack (p, x);
>    }</span >
// >
// <span class="quote">>    static T unpack (const unsigned char *p)
>    {
// >      T x = (0xffU & (T)p[sizeof(T) - N]) << ((N - 1) * __CHAR_BIT__);
// >      return x | Packer<T, N - 1>::unpack (p);
>    }
> };</span >
// >
// <span class="quote">> template <class T>
// > struct PackRule::Packer<T, 0>
> {
>    static void pack (unsigned char*, T) { }
>    static T unpack (const unsigned char*) { return T (); }
> };</span >
// >
// <span class="quote">> int main ()
> {
// >    PackRule p(0, 4);
// >    unsigned char buf[4];
// >    p.pack (buf, (unsigned int)444);
// >    unsigned int t;
// >    p.unpack (buf, &t);
> }</span >
// >


