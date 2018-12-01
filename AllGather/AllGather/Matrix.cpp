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
	return test_values_a;
}

int* permute_a_by_d(int a[], int d,int n) {
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
	int my_curr_a;
	int my_curr_b;


	MPI_Request request, request2;

	//Master process reads the matrices and prepares them for processing
	if (world_rank == 0) {
		int m_size;
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

		int **new_test_values_b = create_sq_matrix(m_size);
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

		int nr_procs = world_size;
		int mesh_size = sqrt(nr_procs);
		int block_size = m_size / mesh_size;
		
		int** proc_ids = create_sq_matrix(mesh_size);
		int proc_id = 0;
		for (int row = 0; row < mesh_size; row++)
			for (int col = 0; col < mesh_size; col++)
				proc_ids[row][col] = proc_id++;

	for (int row = 0; row < mesh_size; row++)
		for (int col = 0; col < mesh_size; col++) {
			int target_proc = proc_ids[row][col];
				v_stanga = (col > 0) ? proc_ids[row][col - 1] : proc_ids[row][mesh_size-1];
				v_dreapta = (col < mesh_size - 1) ? proc_ids[row][col + 1] : proc_ids[row][0];
				v_sus = (row > 0) ? proc_ids[row - 1][col] : proc_ids[mesh_size-1][col];
				v_jos = (row < mesh_size - 1) ? proc_ids[row + 1][col] : proc_ids[0][col];
				//printf("Vecinii lui %d sunt st :%d, dr:%d, sus:%d, jos:%d\n", target_proc, v_stanga, v_dreapta, v_sus, v_jos);
				if (target_proc != 0) {
					data[0] = v_stanga; data[1] = v_dreapta; data[2] = v_sus; data[3] = v_jos;
					MPI_Send(data, 4, MPI_INT, target_proc, 0, MPI_COMM_WORLD );	
					MPI_Send(&new_test_values_a[target_proc / mesh_size][target_proc % mesh_size], 1, MPI_INT, target_proc, 0, MPI_COMM_WORLD);
					MPI_Send(&new_test_values_b[target_proc / mesh_size][target_proc % mesh_size], 1, MPI_INT, target_proc, 0, MPI_COMM_WORLD);
				}
			}
	}
	//end of master preprocess of data
	else {
		MPI_Recv(data, 4, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(&my_curr_a, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(&my_curr_b, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		printf("Vecinii lui %d sunt st :%d, dr:%d, sus:%d, jos:%d a:%d, b:%d \n", world_rank, data[0], data[1], data[2], data[3], my_curr_a, my_curr_b);
	}


/*	//if even, send -> receive
	if (world_rank % 2 == 0) {
	MPI_Send(&my_curr_a, 1, MPI_INT, v_stanga, 0, MPI_COMM_WORLD);
	MPI_Send(&my_curr_b, 1, MPI_INT, v_sus, 0, MPI_COMM_WORLD);
	if (world_rank == 0)
	printf("Proc %d, SENT message\n", world_rank);

	MPI_Recv(&my_curr_a, 1, MPI_INT, v_dreapta, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	if (world_rank == 0)
	printf("RECEIVED number %d from right neighbor %d\n", my_curr_a, v_dreapta);

	MPI_Recv(&my_curr_b, 1, MPI_INT, v_jos, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	if (world_rank == 0)
	printf("RECEIVED number %d from right neighbor %d\n", my_curr_b, v_jos);
	}
	//if odd, receive -> send
	else {
		int temp_a;
		int temp_b;
	//printf("Proc %d, RECEIVING and then SENDING\n", world_rank);
	MPI_Recv(&temp_a, 1, MPI_INT, v_dreapta, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	//printf("Received number %d from right neighbor %d\n", temp_a, v_dreapta);

	MPI_Recv(&temp_b, 1, MPI_INT, v_jos, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	//printf("Received number %d from right neighbor %d\n", temp_b, v_jos);

	MPI_Send(&my_curr_a, 1, MPI_INT, v_stanga, 0, MPI_COMM_WORLD);
	MPI_Send(&my_curr_b, 1, MPI_INT, v_sus, 0, MPI_COMM_WORLD);
	my_curr_a = temp_a;
	my_curr_b = temp_b;
	}

	*/
	
	MPI_Finalize();
	//getchar();
	//return 0;
}


