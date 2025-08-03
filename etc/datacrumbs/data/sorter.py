import json

def sort_json_file_inplace(filename):
    """
    Sort a JSON array file and update the original file with the sorted data.
    
    Args:
        filename (str): Path to the JSON file to sort
    
    Returns:
        bool: True if successful, False if error occurred
    """
    try:
        # Read the JSON file
        with open(filename, 'r') as f:
            data = json.load(f)
        
        # Check if data is a list
        if not isinstance(data, list):
            print(f"Error: JSON file must contain an array/list, found {type(data)}")
            return False
        
        # Sort the array
        sorted_data = sorted(data)
        
        # Write sorted data back to the same file
        with open(filename, 'w') as f:
            json.dump(sorted_data, f, indent=4)
        
        print(f"Successfully sorted {len(sorted_data)} items in {filename}")
        return True
        
    except FileNotFoundError:
        print(f"Error: File '{filename}' not found")
        return False
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON in file '{filename}': {e}")
        return False
    except Exception as e:
        print(f"Error: {str(e)}")
        return False

# Update your specific file
if __name__ == "__main__":
    success = sort_json_file_inplace('/home/haridev/datacrumbs/etc/datacrumbs/data/temp.json')
    if success:
        print("File updated successfully!")
    else:
        print("Failed to update file.")