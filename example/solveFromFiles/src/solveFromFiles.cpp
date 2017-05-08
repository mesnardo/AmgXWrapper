/**
 * @file solveFromFiles.cpp
 * @brief An example and benchmark of AmgX and PETSc
 * @author Pi-Yueh Chuang (pychuang@gwu.edu)
 * @version beta
 * @date 2015-02-01
 */
# include "headers.hpp"

static std::string help = "Test PETSc solver.";

int main(int argc, char **argv)
{
    StructArgs          args;   // a structure containing CMD arguments


    Vec                 u,      // unknowns
                        rhs,    // RHS
                        u_exact,// exact solution
                        err;    // errors

    Mat                 A;      // coefficient matrix

    KSP                 ksp;    // PETSc KSP solver instance

    PetscErrorCode      ierr;   // error codes returned by PETSc routines

    PetscMPIInt         size,   // MPI size
                        myRank; // rank of current process

    PetscClassId        solvingID,
                        warmUpID;

    PetscLogEvent       solvingEvent,
                        warmUpEvent;





    // initialize PETSc and MPI
    ierr = PetscInitialize(&argc, &argv, nullptr, help.c_str());             CHK;
    ierr = PetscLogDefaultBegin();                                           CHK;


    // obtain the rank and size of MPI
    ierr = MPI_Comm_size(PETSC_COMM_WORLD, &size);                           CHK;
    ierr = MPI_Comm_rank(PETSC_COMM_WORLD, &myRank);                         CHK;


    // get parameters from command-line arguments
    ierr = args.getArgs();                                                   CHK;


    // pring case information
    {
        ierr = PetscPrintf(PETSC_COMM_WORLD, "\n");                          CHK;
        for(int i=0; i<72; ++i) ierr = PetscPrintf(PETSC_COMM_WORLD, "=");
        ierr = PetscPrintf(PETSC_COMM_WORLD, "\n");                          CHK;
        for(int i=0; i<72; ++i) ierr = PetscPrintf(PETSC_COMM_WORLD, "-");
        ierr = PetscPrintf(PETSC_COMM_WORLD, "\n");                          CHK;

        if (myRank == 0) args.print();
        ierr = MPI_Barrier(PETSC_COMM_WORLD);                                CHK;

        for(int i=0; i<72; ++i) ierr = PetscPrintf(PETSC_COMM_WORLD, "-");
        ierr = PetscPrintf(PETSC_COMM_WORLD, "\n");                          CHK;
        for(int i=0; i<72; ++i) ierr = PetscPrintf(PETSC_COMM_WORLD, "=");
        ierr = PetscPrintf(PETSC_COMM_WORLD, "\n");                          CHK;

    }


    // create matrix A and load from file
    ierr = readMat(A, args.matrixFileName, "A");                             CHK;


    // create vector rhs and load from file
    ierr = readVec(rhs, args.rhsFileName, "RHS");                            CHK;

    // create vectors u and set to zeros
    {
        ierr = VecDuplicate(rhs, &u);                                        CHK;
        ierr = PetscObjectSetName((PetscObject) u, "unknowns");              CHK;
        ierr = VecSet(u, 0.0);                                               CHK;
    }

    // create vectors err and set to zeros
    {
        ierr = VecDuplicate(rhs, &err);                                      CHK;
        ierr = PetscObjectSetName((PetscObject) err, "errors");              CHK;
        ierr = VecSet(err, 0.0);                                             CHK;
    }


    // register a PETSc event for warm-up and solving
    ierr = PetscClassIdRegister("SolvingClass", &solvingID);                 CHK;
    ierr = PetscClassIdRegister("WarmUpClass", &warmUpID);                   CHK;
    ierr = PetscLogEventRegister("Solving", solvingID, &solvingEvent);       CHK;
    ierr = PetscLogEventRegister("WarmUp", warmUpID, &warmUpEvent);          CHK;

    // create a solver and solve based whether it is AmgX or PETSc
    ierr = createKSP(ksp, A, args.cfgFileName);                          CHK;

    ierr = solve(ksp, A, u, rhs, err, 
            args, warmUpEvent, solvingEvent);                            CHK;

    // destroy KSP
    ierr = KSPDestroy(&ksp);                                             CHK;

    // output a file for petsc performance
    if (args.optFileBool == PETSC_TRUE)
    {
        PetscViewer         viewer; // PETSc viewer

        std::strcat(args.optFileName ,".log");

        ierr = PetscViewerASCIIOpen(
                PETSC_COMM_WORLD, args.optFileName, &viewer);                CHK;
        ierr = PetscLogView(viewer);                                         CHK;
        ierr = PetscViewerDestroy(&viewer);                                  CHK;
    }
    

    // destroy vectors, matrix, dmda
    {
        ierr = VecDestroy(&u);                                               CHK;
        ierr = VecDestroy(&rhs);                                             CHK;
        ierr = VecDestroy(&err);                                             CHK;

        ierr = MatDestroy(&A);                                               CHK;
    }


    {
        ierr = PetscPrintf(PETSC_COMM_WORLD, "\n");                          CHK;
        for(int i=0; i<72; ++i) ierr = PetscPrintf(PETSC_COMM_WORLD, "=");
        ierr = PetscPrintf(PETSC_COMM_WORLD, "\n");                          CHK;
        for(int i=0; i<72; ++i) ierr = PetscPrintf(PETSC_COMM_WORLD, "-");
        ierr = PetscPrintf(PETSC_COMM_WORLD, "\n");                          CHK;
        ierr = PetscPrintf(PETSC_COMM_WORLD, 
                "End of %s\n", args.caseName);                               CHK;
        for(int i=0; i<72; ++i) ierr = PetscPrintf(PETSC_COMM_WORLD, "-");
        ierr = PetscPrintf(PETSC_COMM_WORLD, "\n");                          CHK;
        for(int i=0; i<72; ++i) ierr = PetscPrintf(PETSC_COMM_WORLD, "=");
        ierr = PetscPrintf(PETSC_COMM_WORLD, "\n");                          CHK;
    }

    // finalize PETSc
    ierr = PetscFinalize();                                                  CHK;

    return 0;
}

