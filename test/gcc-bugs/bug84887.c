/* GCC Bug #84887 - missing semicolon: further improvements
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=84887
 */
/* { dg-do compile } */


int test(void)
{
  return 42
} /* { dg-error "expected" } */
// Prior to gcc 8 we emitted:
// t.c: In function ‘test’:
// t.c:4:1: error: expected ‘;’ before ‘}’ token
// }
//  ^
// As of gcc 8 we now emit the error at the correct location:
// t.c: In function ‘test’:
// t.c:3:12: error: expected ‘;’ before ‘}’ token
//    return 42
//              ^
//             ;
//  }
//  ~
// (this was <a class="bz_bug_link 
//           bz_status_RESOLVED  bz_closed"
//    title="RESOLVED FIXED - wrong line for missing semicolon after expression"
//    href="show_bug.cgi?id=65137">PR 65137</a> and others)
// However, as suggested e.g. by oridb on Reddit, it would be more readable to talk about the previous logical unit, and emit:
// t.c: In function ‘test’:
// t.c:3:12: error: expected ‘;’ after ‘42’ token
//    return 42
//              ^
//             ;
// or somesuch: e.g. should we highlight the preceding token as a secondary range, which would give:
// t.c: In function ‘test’:
// t.c:3:12: error: expected ‘;’ after ‘42’ token
//    return 42
//            ~~^
//             ;

// (I'm not sure either way)


