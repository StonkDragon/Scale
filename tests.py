from genericpath import exists
import os
import sys
import asyncio

def genTest(file):
    print(f"Generating Tests for: {file}")
    test_file = file
    ret = os.system("sclc " + test_file)
    if ret != 0:
        print(f"Error generating tests for: {file}")
        sys.exit(1)
    output = os.popen("./out.scl").read()
    if exists("output.txt"):
        os.remove("output.txt")
    with open("output.txt", "w") as f:
        f.write(output)

def runTest(file, directory="examples", current=1, total=1):
    global failedTests
    global theseTestsFailed
    global passedTests
    global skippedTests

    test_file = directory + "/" + file
    curDir = os.path.abspath(os.path.curdir)
    os.chdir(test_file)
    print(f"[COMP] {file}")
    compOut = os.popen("sclc main.scale").read()

    if exists("./out.scl"):
        print(f"[RUN] {file} ({current}/{total})")
        output = os.popen("./out.scl").read()
        
        if not exists("output.txt"):
            print(f"[SKIP] {file}")
            genTest(f"main.scale")
            skippedTests += 1
            os.chdir(curDir)
            return
        
        with open("output.txt", "r") as f:
            expected = f.read()
        
        os.remove("./out.scl")
        os.chdir(curDir)
        
        if output == expected:
            print(f"[PASS] {file}")
            passedTests += 1
            return
        
        print(f"Program Output (characters: {len(output)}):")
        print(output)
        print(f"Expected Output (characters: {len(expected)}):")
        print(expected)
            
    os.chdir(curDir)
    print(f"\b[FAIL] {file}")
    failedTests += 1
    theseTestsFailed.append(file)
    print("Compiler Output:")
    print(compOut)

# loop over every file in the directory examples
# and run the tests on each file
def run_tests(directory):
    tests = [ i for i in os.listdir(directory) if i[0] != '.' ]
    tests.sort()
    current = 1
    try:
        for file in tests:
            runTest(file, current=current, total=len(tests))
            current += 1
    except KeyboardInterrupt:
        pass

    total = passedTests + failedTests + skippedTests
    print("Passed: " + str(passedTests) + "/" + str(total))
    print("Failed: " + str(failedTests) + "/" + str(total) + " " + str(theseTestsFailed))
    print("Skipped: " + str(skippedTests) + "/" + str(total))

# Reset the tests
def reset_tests(directory):
    tests = [ i for i in os.listdir(directory) if i.endswith(".scale") ]
    tests.sort()
    for file in tests:
        genTest(f"{directory}/{file}")

if __name__ == '__main__':
    failedTests = 0
    theseTestsFailed = []
    passedTests = 0
    skippedTests = 0
    
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
