// Author: Wes Kendall
// Copyright 2012 www.mpitutorial.com
// This code is provided freely with the tutorials on mpitutorial.com. Feel
// free to modify it for your own use. Any distribution of the code must
// either provide a link to www.mpitutorial.com or keep this header intact.
//
// Program that computes the average of an array of elements in parallel using
// MPI_Scatter and MPI_Allgather
//

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include <assert.h>
#include <math.h>
#include <fstream>
#include <iostream>
using namespace std;


int** create_sq_matrix(int n) {
	int **test_values_a = (int**)malloc(n*(sizeof(int*)));
	for (int i = 0; i < n; i++)
		test_values_a[i] = (int*)malloc(n * sizeof(int));
	for (int i = 0; i < n; i++)
		for (int j = 0; j < n; j++)
			test_values_a[i][j] = 0;
	return test_values_a;
}

void print_array(int* to_print, int n) {
	for (int i = 0; i < n; i++)
		printf("%d ", to_print[i]);
	printf("\n");
	fflush(stdout);
}

int* permute_a_by_d(int a[], int d, int n) {
	int* new_array = (int*)malloc(n * sizeof(int));
	for (int i = 0; i < n; i++)
		new_array[i] = a[i];

	for (int i = 0; i < d; i++) {

		int temp = new_array[0], j;
		for (j = 0; j < n - 1; j++)
			new_array[j] = new_array[j + 1];

		new_array[j] = temp;
	}
	return new_array;
}

int* get_matrix_partition(int** matrix, int n, int target_proc, int block_size) {
	int* matrix_partition = (int*)malloc(block_size * sizeof(int));
	int start_row = target_proc / n * sqrt(block_size);
	int start_col = target_proc % n * sqrt(block_size);

	int nr_elems = 0;
	for (int i = start_row; i < start_row + sqrt(block_size); i++)
		for (int j = start_col; j < start_col + sqrt(block_size); j++) {
			matrix_partition[nr_elems++] = matrix[i][j];
		}

	//print_array(matrix_partition, block_size);
	return matrix_partition;
}

void print_matrix(int **matrix, int size) {
	for (int i = 0; i < size; i++)
		print_array(matrix[i], size);
	fflush(stdout);
}

void set_matrix_partition(int** matrix, int* source_array, int n, int target_proc, int block_size) {
	int start_row = target_proc / n * sqrt(block_size);
	int start_col = target_proc % n * sqrt(block_size);
	printf("\n %d %d %d\n", start_row, start_col, n);
	int nr_elems = 0;
	for (int i = start_row; i < start_row + sqrt(block_size); i++)
		for (int j = start_col; j < start_col + sqrt(block_size); j++) {
			matrix[i][j] = source_array[nr_elems++];
		}

	//print_matrix(matrix, n);
}

int** create_matrix_from_array(int* partition, int block_size) {
	int n = sqrt(block_size);
	int** new_matrix = create_sq_matrix(n);
	for (int i = 0; i < block_size; i++)
		new_matrix[i / n][i%n] = partition[i];
	return new_matrix;
}


int** multiply_matrices(int** a, int** b, int size) {
	int** result = create_sq_matrix(size);
	for (int i = 0; i < size; ++i)
		for (int j = 0; j < size; ++j)
			for (int k = 0; k < size; ++k)
			{
				result[i][j] += a[i][k] * b[k][j];
			}
	return result;
}


int** add_matrices(int** a, int **b, int size) {
	int** result = create_sq_matrix(size);
	for (int i = 0; i < size; ++i)
		for (int j = 0; j < size; ++j)
			result[i][j] += a[i][j] + b[i][j];

	return result;
}


// Creates an array of random numbers. Each number has a value from 0 - 1

int main(int argc, char** argv) {
	if (argc != 1) {
		fprintf(stderr, "Usage: avg num_elements_per_proc\n");
		exit(1);
	}
	MPI_Init(NULL, NULL);

	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	int v_stanga;
	int v_dreapta;
	int v_sus;
	int v_jos;
	int *data = (int*)malloc(4 * sizeof(int));
	int* my_curr_a;
	int* my_curr_b;
	int mesh_size;
	int block_size;
	int **my_curr_result;
	int m_size;

	MPI_Request request, request2;

	//Master process reads the matrices and prepares them for processing
	if (world_rank == 0) {
		ifstream f("matrix_A.txt");
		f >> m_size;
		int **test_values_a = create_sq_matrix(m_size);
		for (int i = 0; i < m_size; i++)
			for (int j = 0; j < m_size; j++)
				f >> test_values_a[i][j];


		ifstream g("matrix_B.txt");
		g >> m_size;
		int **test_values_b = create_sq_matrix(m_size);
		for (int i = 0; i < m_size; i++)
			for (int j = 0; j < m_size; j++)
				g >> test_values_b[i][j];


		int nr_procs = world_size;
		mesh_size = sqrt(nr_procs);
		block_size = m_size * m_size / (mesh_size*mesh_size);

		my_curr_result = create_sq_matrix(sqrt(block_size));

		int** proc_ids = create_sq_matrix(mesh_size);
		int proc_id = 0;
		for (int row = 0; row < mesh_size; row++)
			for (int col = 0; col < mesh_size; col++)
				proc_ids[row][col] = proc_id++;


		for (int row = 0; row < mesh_size; row++)
			for (int col = 0; col < mesh_size; col++) {
				int target_proc = proc_ids[row][col];
				v_stanga = (col > 0) ? proc_ids[row][col - 1] : proc_ids[row][mesh_size - 1];
				v_dreapta = (col < mesh_size - 1) ? proc_ids[row][col + 1] : proc_ids[row][0];
				v_sus = (row > 0) ? proc_ids[row - 1][col] : proc_ids[mesh_size - 1][col];
				v_jos = (row < mesh_size - 1) ? proc_ids[row + 1][col] : proc_ids[0][col];
				//printf("Vecinii lui %d sunt st :%d, dr:%d, sus:%d, jos:%d\n", target_proc, v_stanga, v_dreapta, v_sus, v_jos);
				if (target_proc != 0) {
					data[0] = v_stanga; data[1] = v_dreapta; data[2] = v_sus; data[3] = v_jos;
					MPI_Send(data, 4, MPI_INT, target_proc, 0, MPI_COMM_WORLD);

					int row = target_proc / mesh_size;
					int col = target_proc % mesh_size;

					int startingIA = (row*mesh_size + (col + mesh_size + row) % mesh_size);
					int startingIB = (((row + mesh_size + col) % mesh_size) *mesh_size) + col;

					int* a_partition = get_matrix_partition(test_values_a, mesh_size, startingIA, block_size);
					int* b_partition = get_matrix_partition(test_values_b, mesh_size, startingIB, block_size);
					MPI_Send(&mesh_size, 1, MPI_INT, target_proc, 0, MPI_COMM_WORLD);
					MPI_Send(&block_size, 1, MPI_INT, target_proc, 0, MPI_COMM_WORLD);
					MPI_Send(a_partition, block_size, MPI_INT, target_proc, 0, MPI_COMM_WORLD);
					MPI_Send(b_partition, block_size, MPI_INT, target_proc, 0, MPI_COMM_WORLD);
				}
			}
		v_stanga = proc_ids[0][mesh_size - 1];
		v_dreapta = proc_ids[0][1];
		v_sus = proc_ids[mesh_size - 1][0];
		v_jos = proc_ids[1][0];
		my_curr_a = get_matrix_partition(test_values_a, m_size, 0, block_size);
		my_curr_b = get_matrix_partition(test_values_b, m_size, 0, block_size);
		int** mat_a = create_matrix_from_array(my_curr_a, block_size);
		int** mat_b = create_matrix_from_array(my_curr_b, block_size);
		my_curr_result = add_matrices(my_curr_result, multiply_matrices(mat_a, mat_b, sqrt(block_size)), sqrt(block_size));
	}

	//end of master preprocess of data
	else {
		MPI_Recv(data, 4, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		v_stanga = data[0]; v_dreapta = data[1]; v_sus = data[2]; v_jos = data[3];
		MPI_Recv(&mesh_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(&block_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		my_curr_a = (int*)malloc(block_size * sizeof(int));
		my_curr_b = (int*)malloc(block_size * sizeof(int));
		my_curr_result = create_sq_matrix(sqrt(block_size));
		MPI_Recv(my_curr_a, block_size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(my_curr_b, block_size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		int** mat_a = create_matrix_from_array(my_curr_a, block_size);
		int** mat_b = create_matrix_from_array(my_curr_b, block_size);
		my_curr_result = add_matrices(my_curr_result, multiply_matrices(mat_a, mat_b, sqrt(block_size)), sqrt(block_size));

	}

	int* temp_a = (int*)malloc(block_size * sizeof(int));;
	int* temp_b = (int*)malloc(block_size * sizeof(int));;
	//if even, send -> receive

	int row_index = world_rank / mesh_size;
	int col_index = world_rank % mesh_size;

	for (int i = 0; i < mesh_size - 1; i++) {
		//Sending A
		if (col_index % 2 == 0) {
			MPI_Send(my_curr_a, block_size, MPI_INT, v_stanga, 0, MPI_COMM_WORLD);
			//printf("Proc %d, SENT message with A to %d\n", world_rank, v_stanga);
			fflush(stdout);

			MPI_Recv(my_curr_a, block_size, MPI_INT, v_dreapta, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			//printf("Proc %d, RECEIVED message with A from %d \n", world_rank, v_dreapta);
			fflush(stdout);
		}
		else {
			MPI_Recv(temp_a, block_size, MPI_INT, v_dreapta, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			//printf("Proc %d, RECEIVED message with A from %d \n", world_rank, v_dreapta);
			fflush(stdout);
			MPI_Send(my_curr_a, block_size, MPI_INT, v_stanga, 0, MPI_COMM_WORLD);
			//printf("Proc %d, SENT message with A to %d\n", world_rank, v_stanga);
			my_curr_a = temp_a;
		}
		MPI_Barrier(MPI_COMM_WORLD);
		//Sending B
		if (row_index % 2 == 0) {
			MPI_Send(my_curr_b, block_size, MPI_INT, v_sus, 0, MPI_COMM_WORLD);
			//printf("Proc %d, SENT message with B to %d\n", world_rank, v_sus);
			fflush(stdout);

			MPI_Recv(my_curr_b, block_size, MPI_INT, v_jos, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			//printf("Proc %d, RECEIVED message with B from %d \n", world_rank, v_jos);
			fflush(stdout);
		}

		else {
			MPI_Recv(temp_b, block_size, MPI_INT, v_jos, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			//printf("Proc %d, RECEIVED message with B from %d \n", world_rank, v_jos);
			fflush(stdout);
			MPI_Send(my_curr_b, block_size, MPI_INT, v_sus, 0, MPI_COMM_WORLD);
			//printf("Proc %d, SENT message with B to %d\n", world_rank, v_sus);
			fflush(stdout);
			my_curr_b = temp_b;
		}
		int** mat_a_1 = create_matrix_from_array(my_curr_a, block_size);
		int** mat_b_1 = create_matrix_from_array(my_curr_b, block_size);
		my_curr_result = add_matrices(my_curr_result, multiply_matrices(mat_a_1, mat_b_1, sqrt(block_size)), sqrt(block_size));
		MPI_Barrier(MPI_COMM_WORLD);
	}

	int* result_vector = get_matrix_partition(my_curr_result, block_size, 0, block_size);
	if (world_rank != 0) {
		MPI_Send(result_vector, block_size, MPI_INT, 0, 0, MPI_COMM_WORLD);
	}

	else {
		int** final_result = create_sq_matrix(m_size);
		set_matrix_partition(final_result, result_vector, mesh_size, 0, block_size);

		int* result_from_matrix_received = (int*)malloc(block_size * sizeof(int));
		for (int i = 1; i < world_size; i++) {
			MPI_Recv(result_from_matrix_received, block_size, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			set_matrix_partition(final_result, result_from_matrix_received, mesh_size, i, block_size);
		}

		print_matrix(final_result, m_size);
	}
	MPI_Finalize();
	//return 0;
}


/*int **new_test_values_b = create_sq_matrix(m_size);
		int **new_test_values_a = create_sq_matrix(m_size);

		//Initial displacement
		for (int row = 0; row < m_size; row++)
			new_test_values_a[row] = permute_a_by_d(test_values_a[row], row, m_size);

		int* col_buffer = (int*)malloc(m_size * sizeof(int));
		for (int col = 0; col < m_size; col++) {
			for (int row = 0; row < m_size; row++)
				col_buffer[row] = test_values_b[row][col];
			col_buffer = permute_a_by_d(col_buffer, col, m_size);
			for (int row = 0; row < m_size; row++)
				new_test_values_b[row][col] = col_buffer[row];
		}
*/