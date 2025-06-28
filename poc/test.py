def test_one():
    print("Running test_one")
    test_two()

def test_two():
    print("Running test_two")
    test_three()

def test_three():
    print("Running test_three")

if __name__ == "__main__":
    test_one()