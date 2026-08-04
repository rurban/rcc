/* GCC Bug #87038 - diagnostics: Have -Wjump-misses-init be enabled by -Wall or -Wextra
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87038
 */


goto _bad_content;
//      ^~~~
// setup.c:313:4: note: label '_bad_content' defined here
    _bad_content:
//     ^~~~~~~~~~~~
// setup.c:311:17: note: 'idx' declared here
    unsigned int idx = 0;
//                  ^~~
// (and more)
// anjuta-3.28.0:
// jsparse.c: In function 'print_node':
// jsparse.c:49:4: error: switch jumps over variable initialization [-Werror=jump-misses-init]
    case TOK_RC:
//     ^~~~
// jsparse.c:32:4: note: switch starts here
    switch ((JSTokenType)node->pn_type)
//     ^~~~~~
// jsparse.c:43:9: note: 'k' declared here
     int k = 0;
//          ^
// (and more)
// at-spi2-core-2.30.0:
// ../at-spi2-core-2.30.0/dbind/dbind-any.c: In function 'dbind_any_demarshal':
// ../at-spi2-core-2.30.0/dbind/dbind-any.c:642:5: error: switch jumps over variable initialization [-Werror=jump-misses-init]
     case DBUS_TYPE_VARIANT:
//      ^~~~
// ../at-spi2-core-2.30.0/dbind/dbind-any.c:541:5: note: switch starts here
     switch (**type) {
//      ^~~~~~
// ../at-spi2-core-2.30.0/dbind/dbind-any.c:618:21: note: 'offset' declared here
                 int offset = 0, stralign;
//                      ^~~~~~
// (and more)
// busybox-1.29.3:
// archival/bbunzip.c: In function 'bbunpack':
// archival/bbunzip.c:68:6: error: jump skips variable initialization [-Werror=jump-misses-init]
      goto free_name;
//       ^~~~
// archival/bbunzip.c:183:2: note: label 'free_name' defined here
  free_name:
//   ^~~~~~~~~
// archival/bbunzip.c:139:10: note: 'del' declared here
    char *del = new_name;
//           ^~~
// In file included from archival/bzip2.c:101:
// archival/libarchive/bz/compress.c: In function 'generateMTFValues':
// archival/libarchive/bz/compress.c:286:3: error: jump skips variable initialization [-Werror=jump-misses-init]
//    goto process_zPend; /* "process it and come back here" */
//    ^~~~
// archival/libarchive/bz/compress.c:251:2: note: label 'process_zPend' defined here
  process_zPend:
//   ^~~~~~~~~~~~~
// archival/libarchive/bz/compress.c:235:11: note: 'll_i' declared here
   uint8_t ll_i = ll_i; /* gcc 4.3.1 thinks it may be used w/o init */
//            ^~~~
// cargo-vendor-0.1.15:
// /h/cargo-vendor-src-0.1.15/vendor/libssh2-sys/libssh2/src/packet.c: In function '_libssh2_packet_add':
// /h/cargo-vendor-src-0.1.15/vendor/libssh2-sys/libssh2/src/packet.c:454:9: error: jump skips variable initialization [-Werror=jump-misses-init]
         goto libssh2_packet_add_jump_point5;
//          ^~~~
// /h/cargo-vendor-src-0.1.15/vendor/libssh2-sys/libssh2/src/packet.c:588:19: note: label 'libssh2_packet_add_jump_point5' defined here
                   libssh2_packet_add_jump_point5:
//                    ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// /h/cargo-vendor-src-0.1.15/vendor/libssh2-sys/libssh2/src/packet.c:574:31: note: 'want_reply' declared here
                 unsigned char want_reply=0;
//                                ^~~~~~~~~~
// (and more)
// chromium-71.0.3578.10:
// ../../third_party/angle/third_party/vulkan-loader/src/loader/trampoline.c: In function 'vkEnumeratePhysicalDevices':
// ../../third_party/angle/third_party/vulkan-loader/src/loader/trampoline.c:658:9: error: jump skips variable initialization [-Werror=jump-misses-init]
         goto out;
//          ^~~~
// ../../third_party/angle/third_party/vulkan-loader/src/loader/trampoline.c:696:1: note: label 'out' defined here
 out:
//  ^~~
// ../../third_party/angle/third_party/vulkan-loader/src/loader/trampoline.c:671:14: note: 'setup_res' declared here
     VkResult setup_res = setupLoaderTrampPhysDevs(instance);
//               ^~~~~~~~~
// (and more)
// efivar-36:
// efivarfs.c: In function 'efivarfs_get_variable_size':
// efivarfs.c:182:3: error: jump skips variable initialization [-Werror=jump-misses-init]
   goto err;
//    ^~~~
// efivarfs.c:195:1: note: label 'err' defined here
 err:
//  ^~~
// efivarfs.c:185:14: note: 'statbuf' declared here
  struct stat statbuf = { 0, };
//               ^~~~~~~
// firefox-63.0b13:
//  1:45.56 In file included from /h/firefox-63.0/third_party/prio/prio/client.c:21:
//  1:45.56 /h/firefox-63.0/third_party/prio/prio/client.c: In function 'share_polynomials':
//  1:45.56 /h/firefox-63.0/third_party/prio/prio/util.h:31:7: error: jump skips variable initialization [-Werror=jump-misses-init]
//  1:45.56        goto cleanup;\
//  1:45.56        ^~~~
//  1:45.56 /h/firefox-63.0/third_party/prio/prio/client.c:98:3: note: in expansion of macro 'P_CHECKA'
//  1:45.56    P_CHECKA (points_g = MPArray_dup (points_f));
//  1:45.56    ^~~~~~~~
//  1:45.56 /h/firefox-63.0/third_party/prio/prio/client.c:143:1: note: label 'cleanup' defined here
//  1:45.56  cleanup:
//  1:45.56  ^~~~~~~
//  1:45.56 /h/firefox-63.0/third_party/prio/prio/client.c:136:7: note: 'j' declared here
//  1:45.56    int j = 0;
//  1:45.56        ^
// (and more)
// ksh-93v:
// /h/ast-93v/src/lib/libast/path/pathopen.c: In function 'pathopen':
// /h/ast-93v/src/lib/libast/path/pathopen.c:248:5: error: jump skips variable initialization [-Werror=jump-misses-init]
     goto adjust;
//      ^~~~
// /h/ast-93v/src/lib/libast/path/pathopen.c:358:3: note: label 'adjust' defined here
   adjust:
//    ^~~~~~
// /h/ast-93v/src/lib/libast/path/pathopen.c:255:10: note: 'server' declared here
    int   server = (oflags&(O_CREAT|O_NOCTTY)) == (O_CREAT|O_NOCTTY);
//           ^~~~~~
// cc1: some warnings being treated as errors
// I've inspected all of these, and I'm convinced they are all false positives so far. In almost all cases, the initialisation has no side effects. In the one exception to that, the side effect is unwanted when the initialisation is skipped. In all these cases, after the label being jumped to, as far as I can tell the variable is not used any more. There is no point in warning about any of this. In one of those cases, the variable is *only* initialised to suppress another GCC false positive!


