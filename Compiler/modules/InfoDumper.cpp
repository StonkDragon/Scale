#include "../headers/Common.hpp"

namespace sclc {

    #define print(s) std::cout << s << std::endl

    void InfoDumper::dump(TPResult result) {
        print("START INFODUMP");
        print("  START CONTAINERS");
        for (Container c : result.containers) {
            print("    CONTAINER: " << c.getName());
            for (Variable v : c.getMembers()) {
                print("      MEMBER: " << v.getName() << ":" << v.getType());
            }
        }
        print("  END CONTAINERS");
        print("  START STRUCTS");
        for (Struct s : result.structs) {
            print("    STRUCT: " << s.getName());
            print("      START IMPLEMENTATIONS");
            for (std::string i : s.getInterfaces()) {
                print("        IMPLEMENTS: " << i);
            }
            print("      END IMPLEMENTATIONS");
            print("      START MEMBERS");
            for (Variable v : s.getMembers()) {
                print("        MEMBER: " << v.getName() << ":" << v.getType());
            }
            print("      END MEMBERS");
            print("      IS_SEALED: " << s.isSealed());
            print("      IS_STATIC: " << s.isStatic());
        }
        print("  END STRUCTS");
        print("  START FUNCTIONS");
        for (Function* f : result.functions) {
            if (f->isMethod)
                continue;
            print("    FUNCTION: " << f->getName() << "(): " << f->getReturnType());
            print("      START ARGUMENTS");
            for (Variable v : f->getArgs()) {
                print("        ARGUMENT: " << v.getName() << ":" << v.getType());
            }
            print("      END ARGUMENTS");
            print("      START BODY");
            for (Token t : f->getBody()) {
                print("        START TOKEN");
                print("          TYPE: " << t.getType());
                if (t.getType() != tok_extern_c)
                    print("          VALUE: " << t.getValue());
                print("          FILE: " << t.getFile());
                print("          LINE: " << t.getLine());
                print("          COLUMN: " << t.getColumn());
                print("        END TOKEN");
            }
            print("      END BODY");
        }
        for (Function* f : result.extern_functions) {
            if (f->isMethod)
                continue;
            print("    FUNCTION: " << f->getName() << "(): " << f->getReturnType());
            print("      MODIFIER EXTERN");
            print("      START ARGUMENTS");
            for (Variable v : f->getArgs()) {
                print("        ARGUMENT: " << v.getName() << ":" << v.getType());
            }
            print("      END ARGUMENTS");
        }
        print("  END FUNCTIONS");
        print("  START METHODS");
        for (Function* m : result.functions) {
            if (!m->isMethod)
                continue;
            Method* f = static_cast<Method*>(m);
            print("    METHOD: " << f->getMemberType() << ":" << f->getName() << "(): " << f->getReturnType());
            print("      START ARGUMENTS");
            for (Variable v : f->getArgs()) {
                if (v.getName() == "self")
                    continue;
                print("        ARGUMENT: " << v.getName() << ":" << v.getType());
            }
            print("      END ARGUMENTS");
            print("      START BODY");
            for (Token t : f->getBody()) {
                print("        START TOKEN");
                print("          TYPE: " << t.getType());
                if (t.getType() != tok_extern_c)
                    print("          VALUE: " << t.getValue());
                print("          FILE: " << t.getFile());
                print("          LINE: " << t.getLine());
                print("          COLUMN: " << t.getColumn());
                print("        END TOKEN");
            }
            print("      END BODY");
        }
        for (Function* m : result.extern_functions) {
            if (!m->isMethod)
                continue;
            Method* f = static_cast<Method*>(m);
            print("    METHOD: " << f->getMemberType() << ":" << f->getName() << "(): " << f->getReturnType());
            print("      MODIFIER EXTERN");
            print("      START ARGUMENTS");
            for (Variable v : f->getArgs()) {
                if (v.getName() == "self")
                    continue;
                print("        ARGUMENT: " << v.getName() << ":" << v.getType());
            }
            print("      END ARGUMENTS");
        }
        print("  END METHODS");
        print("END INFODUMP");
    }
} // namespace sclc
