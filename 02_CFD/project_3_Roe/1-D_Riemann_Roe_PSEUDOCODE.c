/*
    Project 3 pseudocode skeleton:
    Roe scheme for 1-D Euler Riemann problems.

    This file intentionally keeps a Project-2-like structure:
    - one solver struct
    - conservative variables as primary state
    - primitive variables as synchronized derived state
    - method-style function pointers
    - one main marching loop

    It is not intended to compile yet. Fill each TODO(Project 3) block.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define INPUT_LINE_LEN 128
#define OUTPUT_NAME_LEN 128
#define COMMAND_LINE_LEN 1024

typedef struct Riemann_1D_Roe_solver solver;

typedef struct {
    double q1;  /* rho */
    double q2;  /* rho*u */
    double q3;  /* rho*E */
} Conserved;

typedef struct {
    double rho;
    double u;
    double p;
    double E;
    double H;
    double a;
} Primitive;

typedef struct {
    double f1;
    double f2;
    double f3;
} Flux;

struct Riemann_1D_Roe_solver {
    int nx;
    int step_count;
    int output_interval;

    double xmin;
    double xmax;
    double x0;
    double dx;
    double dt;
    double t;
    double t_max;
    double cfl;
    double gamma;

    double left_rho;
    double left_u;
    double left_p;
    double right_rho;
    double right_u;
    double right_p;

    double rho_floor;
    double p_floor;
    double entropy_fix_delta;

    /*
        Main state: cell-centered conserved variables.
        Same idea as Project 2.
    */
    double *q1;
    double *q2;
    double *q3;

    double *q1_next;
    double *q2_next;
    double *q3_next;

    /*
        Interface numerical flux.
        fhat[k] represents Fhat_{k+1/2}, between cell k and k+1.
        Length can be nx - 1.
    */
    double *fhat1;
    double *fhat2;
    double *fhat3;

    /*
        Derived primitive fields for output, CFL, diagnostics.
        They are not advanced independently.
    */
    double *rho;
    double *u;
    double *p;
    double *E;
    double *a;

    int (*allocate)(solver *self);
    void (*bind_methods)(solver *self);
    void (*init_riemann)(solver *self);
    void (*update_primitives)(solver *self);
    double (*compute_dt)(solver *self);
    void (*apply_boundary)(solver *self, double *q1, double *q2, double *q3);
    void (*compute_all_roe_fluxes)(solver *self);
    void (*step_roe)(solver *self);
    int (*check_physical_state)(solver *self);
    void (*write_tecplot)(solver *self, const char *filename);
    void (*destroy)(solver *self);
};

static double pressure_from_q(solver *self, double q1, double q2, double q3)
{
    /*
        TODO(Project 3):
        Same as Project 2.

        kinetic = 0.5 * q2^2 / q1
        p = (gamma - 1) * (q3 - kinetic)
    */
}

static double total_energy_density(solver *self, double rho, double u, double p)
{
    /*
        TODO(Project 3):
        Same as Project 2.

        rhoE = p / (gamma - 1) + 0.5 * rho * u^2
    */
}

static Primitive primitive_from_q(solver *self, Conserved q)
{
    /*
        TODO(Project 3):
        Convert Q -> W and useful thermodynamic quantities.

        rho = q1
        u   = q2 / q1
        p   = pressure_from_q(q1, q2, q3)
        E   = q3 / q1
        H   = (q3 + p) / rho
        a   = sqrt(gamma * p / rho)

        Add guard logic or diagnostics if rho <= 0 or p <= 0.
    */
}

static Flux physical_flux_from_q(solver *self, Conserved q)
{
    /*
        TODO(Project 3):
        Compute physical Euler flux F(Q).

        W = primitive_from_q(q)

        F1 = rho*u
        F2 = rho*u^2 + p
        F3 = u * (rhoE + p)
    */
}

static double entropy_fixed_abs_lambda(double lambda, double delta)
{
    /*
        TODO(Project 3):
        Harten entropy fix.

        if abs(lambda) >= delta:
            return abs(lambda)
        else:
            return 0.5 * (lambda^2 / delta + delta)

        For first implementation, delta can be 0.1 * a_tilde
        or a small user-set constant. Then test sensitivity.
    */
}

static Flux roe_flux(solver *self, Conserved qL, Conserved qR)
{
    /*
        TODO(Project 3):
        This is the core of Project 3.

        Input:
            qL = Q_i
            qR = Q_{i+1}

        Output:
            Fhat_{i+1/2}

        Step 1: compute primitive states.

            WL = primitive_from_q(qL)
            WR = primitive_from_q(qR)

        Step 2: compute physical fluxes.

            FL = physical_flux_from_q(qL)
            FR = physical_flux_from_q(qR)

        Step 3: compute Roe averages.

            sqrt_rho_L = sqrt(rho_L)
            sqrt_rho_R = sqrt(rho_R)
            denom = sqrt_rho_L + sqrt_rho_R

            u_tilde = (sqrt_rho_L*u_L + sqrt_rho_R*u_R) / denom
            H_tilde = (sqrt_rho_L*H_L + sqrt_rho_R*H_R) / denom
            a_tilde = sqrt((gamma - 1) * (H_tilde - 0.5*u_tilde^2))

        Step 4: eigenvalues of Roe matrix.

            lambda1 = u_tilde - a_tilde
            lambda2 = u_tilde
            lambda3 = u_tilde + a_tilde

            Apply entropy fix to abs(lambda_k).

        Step 5: wave strengths.

            dQ = qR - qL
            drho = dQ1
            du   = u_R - u_L
            dp   = p_R - p_L

            One common 1-D Euler form:

            alpha1 = 0.5 * (dp - rho_tilde*a_tilde*du) / (a_tilde^2)
            alpha2 = drho - dp / (a_tilde^2)
            alpha3 = 0.5 * (dp + rho_tilde*a_tilde*du) / (a_tilde^2)

            where rho_tilde = sqrt(rho_L * rho_R).

            Check this formula carefully against your notes/textbook.

        Step 6: right eigenvectors.

            r1 = [1, u_tilde - a_tilde, H_tilde - u_tilde*a_tilde]
            r2 = [1, u_tilde,           0.5*u_tilde^2]
            r3 = [1, u_tilde + a_tilde, H_tilde + u_tilde*a_tilde]

        Step 7: assemble Roe flux.

            Fhat = 0.5*(FL + FR)
                 - 0.5*( |lambda1|*alpha1*r1
                        + |lambda2|*alpha2*r2
                        + |lambda3|*alpha3*r3 )

        Return Fhat.
    */
}

int solver_allocate(solver *self)
{
    /*
        TODO(Project 3):
        Allocate:
            q1, q2, q3
            q1_next, q2_next, q3_next
            fhat1, fhat2, fhat3
            rho, u, p, E, a

        Difference from Project 2:
            no q_bar arrays
            no f_bar arrays
            fhat arrays represent interface fluxes
    */
}

void solver_update_primitives(solver *self)
{
    /*
        TODO(Project 3):
        Same role as Project 2.

        for each cell i:
            Q_i -> W_i
            store rho/u/p/E/a
    */
}

double solver_compute_dt(solver *self)
{
    /*
        TODO(Project 3):
        Same idea as Project 2.

        update_primitives()
        max_speed = max_i(|u_i| + a_i)
        dt = cfl * dx / max_speed

        For first-order explicit Roe, CFL <= 1 is the usual starting point.
    */
}

void solver_apply_boundary(solver *self, double *q1, double *q2, double *q3)
{
    /*
        TODO(Project 3):
        Same transmissive boundary as Project 2.

        left ghost/edge cell copies neighbor:
            q[0] = q[1]

        right:
            q[nx-1] = q[nx-2]

        Note:
        A more formal finite-volume code would use ghost cells.
        This teaching skeleton keeps Project 2's simpler boundary pattern.
    */
}

void solver_init_riemann(solver *self)
{
    /*
        TODO(Project 3):
        Same as Project 2's init_sod, but name it init_riemann
        because the selected case may be Sod, Lax, expansion, contact, etc.

        for each cell center x_i:
            if x_i < x0: use left state
            else:        use right state

            q1 = rho
            q2 = rho*u
            q3 = rhoE

        apply_boundary()
        update_primitives()
    */
}

void solver_compute_all_roe_fluxes(solver *self)
{
    /*
        TODO(Project 3):
        Compute interface fluxes.

        for k = 0 .. nx-2:
            qL = Q_k
            qR = Q_{k+1}
            fhat[k] = roe_flux(qL, qR)

        Index meaning:
            fhat[k] = Fhat_{k+1/2}

        Cell i update later uses:
            right flux = fhat[i]
            left flux  = fhat[i-1]
    */
}

int solver_check_physical_state(solver *self)
{
    /*
        TODO(Project 3):
        After each update:
            if q1[i] <= rho_floor: report failure
            if pressure_from_q(...) <= p_floor: report failure

        Return:
            1 if physical
            0 if failed

        Do not hide instability silently. For a numerical-method report,
        a controlled failure is useful evidence.
    */
}

void solver_step_roe(solver *self)
{
    /*
        TODO(Project 3):
        Roe explicit finite-volume step.

        self->dt = compute_dt()
        if t + dt > t_max:
            dt = t_max - t

        lambda = dt / dx

        apply_boundary(q)
        compute_all_roe_fluxes()

        for i = 1 .. nx-2:
            q_next[i] = q[i] - lambda * (fhat[i] - fhat[i-1])

        apply_boundary(q_next)

        if check_physical_state(q_next) fails:
            stop or mark unstable

        copy q_next -> q
        t += dt
        step_count += 1
        update_primitives()
    */
}

void solver_write_tecplot(solver *self, const char *filename)
{
    /*
        TODO(Project 3):
        Same output contract as Project 2, so postprocess scripts still work.

        VARIABLES = "x", "rho", "u", "p", "E", "q1", "q2", "q3"

        Only change the title:
            TITLE = "1D Euler Roe Result"
    */
}

void solver_destroy(solver *self)
{
    /*
        TODO(Project 3):
        Free every allocated array.
        Then memset(self, 0, sizeof(*self)).
    */
}

void solver_bind_methods(solver *self)
{
    /*
        TODO(Project 3):
        Bind method pointers:

        allocate               -> solver_allocate
        init_riemann           -> solver_init_riemann
        update_primitives      -> solver_update_primitives
        compute_dt             -> solver_compute_dt
        apply_boundary         -> solver_apply_boundary
        compute_all_roe_fluxes -> solver_compute_all_roe_fluxes
        step_roe               -> solver_step_roe
        check_physical_state   -> solver_check_physical_state
        write_tecplot          -> solver_write_tecplot
        destroy                -> solver_destroy
    */
}

static solver solver_create_default(void)
{
    /*
        TODO(Project 3):
        Same defaults as Project 2 baseline:

        nx = 501
        domain = [0, 1]
        x0 = 0.5
        gamma = 1.4
        CFL = 0.5
        t_max = 0.2
        Sod left/right states

        Roe-specific:
            entropy_fix_delta = choose small positive value or compute locally
            rho_floor = 1e-10
            p_floor = 1e-10

        bind_methods()
        return solver
    */
}

static void configure_riemann_case(solver *self, int *case_id)
{
    /*
        TODO(Project 3):
        Copy Project 2's seven Riemann cases.

        This is safe to reuse conceptually because Roe and MacCormack solve
        the same PDE with the same initial data.
    */
}

static void configure_domain_and_grid(solver *self)
{
    /*
        TODO(Project 3):
        Same as Project 2.
        Ask xmin, xmax, x0, nx.
        Then compute dx.
    */
}

static void configure_numerics(solver *self)
{
    /*
        TODO(Project 3):
        Ask gamma, CFL, t_max, output interval.

        Do not ask for artificial viscosity in the Roe baseline.
        Optional later experiment:
            add entropy-fix strength or positivity-fix mode.
    */
}

static int run_exact_solver(const solver *self, double output_time, const char *exact_filename)
{
    /*
        TODO(Project 3):
        Reuse Project 2's external exact-solver process idea.

        Important:
            exact solver must receive the same
            left/right state, gamma, xmin, xmax, x0, nx, and output_time.
    */
}

int main(void)
{
    /*
        Project 3 main flow:

        solver my_solver = solver_create_default()

        print_banner()
        configure_riemann_case()
        configure_domain_and_grid()
        configure_numerics()
        ask_output_filename()
        print_run_summary()

        allocate()
        init_riemann()

        while t < t_max and step_count < step_limit:
            step_roe()
            if output_interval is active:
                write snapshot
                run exact solver at same time

        write final numerical file
        run exact solver at final time
        destroy()

        return
    */
}
