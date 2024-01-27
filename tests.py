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
    if exists(test_file + ".txt"):
        os.remove(test_file + ".txt")
    with open(test_file + ".txt", "w") as f:
        f.write(output)

async def runTest(file, directory="examples", current=1, total=1):
    global failedTests
    global theseTestsFailed
    global passedTests
    global skippedTests

    test_file = directory + "/" + file
    print(f"[COMP] {file}")
    try:
        compOut = os.popen("sclc " + test_file + "/main.scale").read()
    except UnicodeError:
        runTest(file, directory, current, total)
        return

    print(f"[RUN] {file} ({current}/{total})")
    output = os.popen("./out.scl").read()
    if not exists(test_file + "/output.txt"):
        print(f"[SKIP] {file}")
        genTest(test_file)
        skippedTests += 1
        return
    with open(test_file + "/output.txt", "r") as f:
        expected = f.read()
    if output == expected:
        print(f"[PASS] {file}")
        passedTests += 1
    else:
        print(f"\b[FAIL] {file}")
        failedTests += 1
        theseTestsFailed.append(file)
        print("Compiler Output:")
        print(compOut)
        print("Program Output:")
        print(output)
        print("Expected Output:")
        print(expected)
        print("")

# loop over every file in the directory examples
# and run the tests on each file
async def run_tests(directory):
    tests = [ i for i in os.listdir(directory) ]
    tests.sort()
    current = 1
    tasks = []
    for file in tests:
        tasks.append(asyncio.create_task(runTest(file, current=current, total=len(tests))))
        current += 1

    for task in tasks:
        await task

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
            asyncio.run(run_tests("examples"))
        else:
            print("Usage: python3 tests.py [run|reset]")
            sys.exit(1)
