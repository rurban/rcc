// Self-referential object macro: A must expand to B.A, not infinitely
struct BA { int A; };
struct BA B = { 42 };
#define A B.A

// Chained: control_points -> segx[r2].control_points -> paths[r1].segs[r2].control_points
struct CP { int val; };
struct Seg { struct CP *control_points; int num_cp; };
struct Path { struct Seg *segs; };
struct Obj { struct Path *paths; };

#define segx paths[r1].segs
#define control_points segx[r2].control_points

int main() {
    if (A != 42) return 1;

    struct CP cp = { 7 };
    struct Seg seg = { &cp, 1 };
    struct Path path = { &seg };
    struct Obj obj = { &path };
    struct Obj *_obj = &obj;
    int r1 = 0, r2 = 0;
    if (_obj->control_points->val != 7) return 2;

    return 0;
}
