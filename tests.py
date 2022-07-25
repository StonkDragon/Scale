from genericpath import exists
import os
import sys
from venv import create

# loop over every file in the directory examples
# and run the tests on each file
def run_tests(directory):
    failedTests = 0
    passedTests = 0
    skippedTests = 0
    for file in os.listdir(directory):
        if file.endswith(".scale"):
            test_file = directory + "/" + file
            print("[COMP] " + file)
            os.popen("sclc " + test_file).read()
            print("[RUN] " + file)
            output = os.popen("./out.scl").read()
            if not exists(test_file + ".txt"):
                print("[SKIP] " + file)
                skippedTests += 1
                continue
            with open(test_file + ".txt", "r") as f:
                expected = f.read()
            if output == expected:
                print("[PASS] " + file)
                passedTests += 1
            else:
                print("[FAIL] " + file)
                failedTests += 1
                print("Expected output:")
                print(expected)
                print("Actual output:")
                print(output)
                break
    print("Passed: " + str(passedTests))
    print("Failed: " + str(failedTests))
    print("Skipped: " + str(skippedTests))

def reset_tests(directory):
    for file in os.listdir(directory):
        if file.endswith(".scale"):
            print("Generating Tests for: " + file)
            test_file = directory + "/" + file
            os.popen("sclc " + test_file).read()
            output = os.popen("./out.scl").read()
            if exists(test_file + ".txt"):
                os.remove(test_file + ".txt")
            with open(test_file + ".txt", "w") as f:
                f.write(output)

if __name__ == '__main__':
    if len(sys.argv) != 2:
        run_tests("examples")
    else:
        if sys.argv[1] == "reset":
            reset_tests("examples")
        elif sys.argv[1] == "run":
            run_tests("examples")
        else:
            print("Usage: python3 tests.py [run|reset]")
            sys.exit(1)
