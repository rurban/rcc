/* A cast's VM (variably-modified) array-length expression must be
 * evaluated exactly once, even when the VM array sits behind a function
 * return type in the pointer chain, e.g. `int (*(*)(void))[++l]`
 * (pointer to function returning pointer to VM array). vla_freeze_dims()
 * only recursed through TY_PTR/TY_VLA ->base chains, so it never reached
 * the VLA nested inside a TY_FUNC's return_ty, and ++l silently never
 * ran. From michaelforney/cproc's test/cast-vm.c. */
int main(void) {
    int l = 0;
    (int (*)[++l])0;
    (int (*(*)(void))[++l])0;
    return l != 2;
}
