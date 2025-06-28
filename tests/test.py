import os

def test_one():
    print("Running test_one")
    test_two()

def test_two():
    print("Running test_two")
    test_three()

def test_three():
    with open("tempfile.txt", "w") as f:
        f.write("Hello, world!")
    os.remove("tempfile.txt")
    print("Running test_three")

if __name__ == "__main__":
    print(f"PID: {os.getpid()}")
    input("Press Enter to continue...")
    test_one()
    input("Press Enter to continue...")