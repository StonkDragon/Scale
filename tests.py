from genericpath import exists
import os
import sys

def genTest(file):
    print("Generating Tests for: " + file)
    test_file = file
    ret = os.system("sclc " + test_file)
    if ret != 0:
        print("Error generating tests for: " + file)
        sys.exit(1)
    output = os.popen("./out.scl").read()
    if exists(test_file + ".txt"):
        os.remove(test_file + ".txt")
    with open(test_file + ".txt", "w") as f:
        f.write(output)

# loop over every file in the directory examples
# and run the tests on each file
def run_tests(directory):
    failedTests = 0
    theseTestsFailed = []
    passedTests = 0
    skippedTests = 0
    for file in os.listdir(directory):
        if file.endswith(".scale"):
            test_file = directory + "/" + file
            print("[COMP] " + file)
            compOut = os.popen("sclc " + test_file).read()
            print("[RUN] " + file)
            output = os.popen("./out.scl").read()
            if not exists(test_file + ".txt"):
                print("[SKIP] " + file)
                genTest(test_file)
                skippedTests += 1
                continue
            with open(test_file + ".txt", "r") as f:
                expected = f.read()
            if output == expected:
                print("[PASS] " + file)
                passedTests += 1
            else:
                print("\b[FAIL] " + file)
                failedTests += 1
                theseTestsFailed.append(file)
                print("Compiler Output:")
                print(compOut)
                print("Program Output:")
                print(output)
                print("Expected Output:")
                print(expected)
                print("")
    total = passedTests + failedTests + skippedTests
    print("Passed: " + str(passedTests) + "/" + str(total))
    print("Failed: " + str(failedTests) + "/" + str(total) + " " + str(theseTestsFailed))
    print("Skipped: " + str(skippedTests) + "/" + str(total))

# Reset the tests
def reset_tests(directory):
    for file in os.listdir(directory):
        if file.endswith(".scale"):
            genTest(directory + "/" + file)

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: python3 tests.py [run|reset]")
        sys.exit(1)
    else:
        if sys.argv[1] == "reset":
            reset_tests("examples")
        elif sys.argv[1] == "run":
            run_tests("examples")
        else:
            print("Usage: python3 tests.py [run|reset]")
            sys.exit(1)
