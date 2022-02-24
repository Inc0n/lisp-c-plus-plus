

#include <stdio.h>
#include <iostream>

using namespace std;

#include "lisp.hpp"
#include "reader.hpp"

int main() {
    Environment *env = getVM()->root_env;

    while (true) {
        debuglog("before, %d(%d)\n", getVM()->numObjs(), env->count_obj());
        cout << ";;; Eval input:\n";
        Cell *exp = lisp_read(stdin);
        /* exit(1); */
        cout << "\n";
        Cell *result = eval(exp, env);


        cout << ";;; Eval value:\n" << result << "\n";

        /* int freed = destroyObject(getVM(), exp); */
        debuglog("after, %d(%d)\n", getVM()->numObjs(), env->count_obj());
    }
}
