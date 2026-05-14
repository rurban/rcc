// Test TR39 Unicode identifier security via libu8ident.
// TR39 rejects what C11-C26 innocently allows:
//   - Mixed scripts (e.g. Cyrillic + Latin)   -> ERR_SCRIPTS  warning
//   - Limited use scripts (Cherokee, Adlam)    -> ERR_SCRIPT   hard error
//   - Greek letters confusable with Latin      -> ERR_SCRIPTS  warning
//   - Combining mark violations (>4 marks)     -> ERR_COMBINE  warning
//   - Non-spacing mark making base confusable  -> ERR_COMBINE  warning
//
// #pragma unicode ScriptName  allows a script for the rest of the file/scope.
// #pragma unicode reset       resets the script context to default.
//
// Known homoglyphs C11-C26 would allow:
//   Cyrillic 'а' (U+0430) looks like Latin 'a' (U+0061)
//   Cyrillic 'о' (U+043E) looks like Latin 'o' (U+006F)
//   Greek   'ο' (U+03BF) looks like Latin 'o' (U+006F)

#include <stdio.h>
#include <assert.h>

// Test 1: With #pragma unicode Cyrillic, Cyrillic identifiers are allowed.
#pragma unicode Cyrillic
double привет = 0.1;
#pragma unicode reset

// Test 2: Without pragma, Cyrillic+Lefèvre(Latin) triggers mixed script warning.
// But Lefèvre alone with Latin is fine.
int Lefèvre = 2;

// Test 3: Greek+Lefèvre would trigger mixed script warning (Cyrillic disallowed in TR39_4).
// Use Greek with pragma.
#pragma unicode Greek
double λόγος = 3.14;
#pragma unicode reset

// Test 4: CJK with Han+Hiragana+Katakana — these are allowed combinations.
// Japanese: Hiragana + Katakana + Han
#pragma unicode Han
#pragma unicode Hiragana
#pragma unicode Katakana
int 漢字 = 42;
#pragma unicode reset

// Test 5: Arabic with Latin — allowed in TR39_4 (any Recommended + Latin)
#pragma unicode Arabic
int كتاب = 99;
#pragma unicode reset

// Test 6: Function names with Unicode
#pragma unicode Cyrillic
double функции(double x) {
    return x * 2.0;
}
#pragma unicode reset

// Test 7: Parameter names with Unicode
#pragma unicode Cyrillic
static int сложение(int первое, int второе) {
    return первое + второе;
}
#pragma unicode reset

// Test 8: Typedef with Unicode
#pragma unicode Cyrillic
typedef double число;
#pragma unicode reset

// Test 9: Enum constants with Unicode
#pragma unicode Cyrillic
enum цвета {
    КРАСНЫЙ = 1,
    ЗЕЛЁНЫЙ = 2,
    СИНИЙ = 3
};
#pragma unicode reset

// Test 10: Struct member names with Unicode
#pragma unicode Cyrillic
struct точка {
    double x;
    double y;
    char имя[32];
};
#pragma unicode reset

#pragma unicode Cyrillic
struct точка точка1 = { 1.0, 2.0, "test" };
#pragma unicode reset

// Test 13: Macro names with Unicode
#pragma unicode Cyrillic
#define ПРИВЕТ 42
#define УДВОИТЬ(x) ((x) * 2)
#pragma unicode reset

// Test 14: #ifdef/#ifndef/#undef with Unicode macro names
#pragma unicode Cyrillic
#define ТЕСТ_МАКРО 123
#ifdef ТЕСТ_МАКРО
int макро_определён = 1;
#else
int макро_определён = 0;
#endif
#ifndef ТЕСТ_НЕТ
int макро_отсутствует = 1;
#else
int макро_отсутствует = 0;
#endif
#undef ТЕСТ_МАКРО
#ifdef ТЕСТ_МАКРО
int макро_после_undef = 0;
#else
int макро_после_undef = 1;
#endif
#pragma unicode reset

int main() {
#pragma unicode Cyrillic
    printf("привет=%g\n", привет);
    assert(привет == 0.1);
#pragma unicode reset

    printf("Lefèvre=%d\n", Lefèvre);
    assert(Lefèvre == 2);

#pragma unicode Greek
    printf("λόγος=%g\n", λόγος);
    assert(λόγος == 3.14);
#pragma unicode reset

#pragma unicode Han
#pragma unicode Hiragana
#pragma unicode Katakana
    printf("漢字=%d\n", 漢字);
    assert(漢字 == 42);
#pragma unicode reset

#pragma unicode Arabic
    printf("كتاب=%d\n", كتاب);
    assert(كتاب == 99);
#pragma unicode reset

    // Test 6: Function call with Unicode name
#pragma unicode Cyrillic
    double р = функции(5.0);
    assert(р == 10.0);
    printf("функции(5.0)=%g\n", р);
#pragma unicode reset

    // Test 7: Parameter names with Unicode
#pragma unicode Cyrillic
    int сумма = сложение(10, 20);
    assert(сумма == 30);
    printf("сложение(10,20)=%d\n", сумма);
#pragma unicode reset

    // Test 8: Typedef with Unicode
#pragma unicode Cyrillic
    число пи = 3.14159;
    assert(пи > 3.14);
    printf("число пи=%g\n", пи);
#pragma unicode reset

    // Test 9: Enum constants with Unicode
#pragma unicode Cyrillic
    assert(КРАСНЫЙ == 1);
    printf("КРАСНЫЙ=%d\n", КРАСНЫЙ);
#pragma unicode reset

    // Test 10: Struct member names with Unicode
#pragma unicode Cyrillic
    assert(точка1.x == 1.0);
    assert(точка1.y == 2.0);
    printf("точка1.x=%g точка1.y=%g\n", точка1.x, точка1.y);
#pragma unicode reset

    // Test 11: Local variable names with Unicode
#pragma unicode Cyrillic
    {
        int русский = 42;
        assert(русский == 42);
        printf("русский=%d\n", русский);
    }
#pragma unicode reset

    // Test 13: Macro names with Unicode
#pragma unicode Cyrillic
    assert(ПРИВЕТ == 42);
    printf("ПРИВЕТ=%d\n", ПРИВЕТ);
    assert(УДВОИТЬ(21) == 42);
    printf("УДВОИТЬ(21)=%d\n", УДВОИТЬ(21));
#pragma unicode reset

    // Test 14: #ifdef/#ifndef/#undef with Unicode macro names
#pragma unicode Cyrillic
    assert(макро_определён == 1);
    assert(макро_отсутствует == 1);
    assert(макро_после_undef == 1);
    printf("макро_определён=%d макро_отсутствует=%d макро_после_undef=%d\n",
           макро_определён, макро_отсутствует, макро_после_undef);
#pragma unicode reset

    // Test 12: Label names with Unicode (goto)
#pragma unicode Cyrillic
    {
        int метка = 0;
        goto МЕТКА;
        метка = 1;
    МЕТКА:
        assert(метка == 0);
        printf("метка (after goto)=%d\n", метка);
    }
#pragma unicode reset

    return 0;
}
