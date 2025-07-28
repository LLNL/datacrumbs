#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <hdf5.h> // HDF5 library header

#define FILENAME "random_data_c.h5"
#define DATASETNAME "random_data"
#define RANK 1 // Our data is a 1D array

int main() {
    hid_t       file_id;    // File identifier
    hid_t       dataset_id; // Dataset identifier
    hid_t       dataspace_id; // Dataspace identifier
    herr_t      status;     // Error status

    long long   size_mb = 100; // Target size in MB
    long long   bytes_per_element = sizeof(double); // Size of one element (double)
    long long   total_bytes = size_mb * 1024 * 1024;
    hsize_t     num_elements = total_bytes / bytes_per_element; // Number of elements

    double      *data; // Pointer to our data array

    printf("Attempting to create an HDF5 file '%s' with a %lldMB random array...\n", FILENAME, size_mb);

    // Allocate memory for the data array
    data = (double *)malloc(num_elements * bytes_per_element);
    if (data == NULL) {
        fprintf(stderr, "Failed to allocate memory for the array.\n");
        return 1;
    }

    // Initialize random number generator
    srand((unsigned int)time(NULL)); // Seed with current time for more randomness
    // For reproducibility, you could use srand(42);

    printf("Generating a random array with %llu double elements...\n", (unsigned long long)num_elements);
    // Populate the array with random double values (0.0 to 1.0)
    for (hsize_t i = 0; i < num_elements; i++) {
        data[i] = (double)rand() / RAND_MAX;
    }
    printf("Array generated. Actual array size in memory: %.2f MB\n", (double)num_elements * bytes_per_element / (1024 * 1024));

    // Create a new HDF5 file
    file_id = H5Fcreate(FILENAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (file_id < 0) {
        fprintf(stderr, "Failed to create HDF5 file.\n");
        free(data);
        return 1;
    }
    printf("HDF5 file '%s' created.\n", FILENAME);

    // Create the dataspace for the dataset
    // For a 1D array, the dimension array has one element
    hsize_t dims[RANK];
    dims[0] = num_elements;
    dataspace_id = H5Screate_simple(RANK, dims, NULL); // NULL for max_dims means same as dims
    if (dataspace_id < 0) {
        fprintf(stderr, "Failed to create dataspace.\n");
        H5Fclose(file_id);
        free(data);
        return 1;
    }

    // Create the dataset
    dataset_id = H5Dcreate2(file_id, DATASETNAME, H5T_NATIVE_DOUBLE, dataspace_id,
                            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (dataset_id < 0) {
        fprintf(stderr, "Failed to create dataset.\n");
        H5Sclose(dataspace_id);
        H5Fclose(file_id);
        free(data);
        return 1;
    }
    printf("Dataset '%s' created in '%s'.\n", DATASETNAME, FILENAME);

    // Write the data to the dataset
    status = H5Dwrite(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    if (status < 0) {
        fprintf(stderr, "Failed to write data to dataset.\n");
        H5Dclose(dataset_id);
        H5Sclose(dataspace_id);
        H5Fclose(file_id);
        free(data);
        return 1;
    }
    printf("Successfully wrote the random array to '%s'.\n", FILENAME);

    // Close the dataset, dataspace, and file
    status = H5Dclose(dataset_id);
    if (status < 0) fprintf(stderr, "Failed to close dataset.\n");
    status = H5Sclose(dataspace_id);
    if (status < 0) fprintf(stderr, "Failed to close dataspace.\n");
    status = H5Fclose(file_id);
    if (status < 0) fprintf(stderr, "Failed to close file.\n");

    // Free the allocated memory
    free(data);

    printf("Program finished successfully.\n");

    return 0;
}
