/*
    Project 3 pseudocode skeleton:
    Roe scheme for 1-D Euler Riemann problems.

    This file is intentionally shaped like the current Project 2 solver:
    - one solver struct
    - conservative variables Q as the primary state
    - primitive variables W synchronized from Q
    - method-style function pointers
    - interactive configuration
    - snapshot output
    - external Project 0 exact-solver coupling
    - Tecplot ASCII output contract compatible with Project 2 postprocess

    It is not intended to compile yet. Fill each TODO(Project 3) block.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define INPUT_LINE_LEN 128
#define OUTPUT_NAME_LEN 128
#define COMMAND_LINE_LEN 1024
#define EXACT_SOLVER_EXE "_Analysical_Solution_Solver\\riemann_exact.exe"

typedef struct Riemann_1D_Roe_solver solver;

typedef enum {
    ENTROPY_FIX_OFF = 0,
    ENTROPY_FIX_HARTEN = 1
} EntropyFixType;

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
    int failed;

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

    /*
        Project 2 studies artificial viscosity beta/sensor.
        Project 3 Roe should instead study entropy fix and positivity behavior.
    */
    EntropyFixType entropy_fix_type;
    double entropy_fix_factor;  /* often multiplied by local a_tilde */
    double entropy_fix_delta;   /* optional absolute lower value */
    double rho_floor;
    double p_floor;

    /*
        Main state: cell-centered conserved variables.
        Same state philosophy as Project 2.
    */
    double *q1;
    double *q2;
    double *q3;

    double *q1_next;
    double *q2_next;
    double *q3_next;

    /*
        Interface numerical flux.
        fhat[k] = Fhat_{k+1/2}, between cell k and k+1.
        Length can be nx - 1, but allocating nx is also acceptable for
        Project-2-style simplicity.
    */
    double *fhat1;
    double *fhat2;
    double *fhat3;

    /*
        Derived state for CFL, output, diagnostics, and postprocess.
        W is not advanced independently.
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
    int (*check_physical_state)(solver *self, const double *q1, const double *q2, const double *q3);
    void (*write_tecplot)(solver *self, const char *filename);
    void (*destroy)(solver *self);
};

static double pressure_from_q(solver *self, double q1, double q2, double q3)
{
    /*
        TODO(Project 3):
        Same formula as Project 2.

        kinetic = 0.5 * q2 * q2 / q1
        p = (gamma - 1) * (q3 - kinetic)
    */
}

static double total_energy_density(solver *self, double rho, double u, double p)
{
    /*
        TODO(Project 3):
        Same formula as Project 2.

        rhoE = p / (gamma - 1) + 0.5 * rho * u * u
    */
}

static double *alloc_double_array(size_t n)
{
    /*
        TODO(Project 3):
        Reuse Project 2's calloc style.
        calloc is preferable here because it gives zero initialization and
        safer multiplication behavior than plain malloc(n * sizeof(double)).
    */
}

static Conserved conserved_at(const double *q1, const double *q2, const double *q3, int i)
{
    /*
        TODO(Project 3):
        Convenience helper:
            return (Conserved){q1[i], q2[i], q3[i]};
    */
}

static Primitive primitive_from_q(solver *self, Conserved q)
{
    /*
        TODO(Project 3):
        Convert one conservative state to primitive/thermodynamic state.

        rho = q.q1
        u   = q.q2 / q.q1
        p   = pressure_from_q(self, q.q1, q.q2, q.q3)
        E   = q.q3 / q.q1
        H   = (q.q3 + p) / rho
        a   = sqrt(gamma * p / rho)

        If rho <= 0 or p <= 0, do not silently repair during baseline.
        Mark failure through check_physical_state() so the parameter-matrix
        driver can record "unstable".
    */
}

static Flux physical_flux_from_q(solver *self, Conserved q)
{
    /*
        TODO(Project 3):
        Compute Euler physical flux F(Q).

        W = primitive_from_q(self, q)

        F1 = q.q2
        F2 = q.q2 * W.u + W.p
        F3 = (q.q3 + W.p) * W.u

        This replaces Project 2's array-based compute_flux() as a local helper.
        Roe still needs interface flux, not just this cell flux.
    */
}

static double entropy_fixed_abs_lambda(
    solver *self,
    double lambda,
    double local_sound_speed
) {
    /*
        TODO(Project 3):
        Harten entropy fix for one eigenvalue.

        if entropy_fix_type == ENTROPY_FIX_OFF:
            return fabs(lambda)

        delta = max(entropy_fix_delta, entropy_fix_factor * local_sound_speed)

        if fabs(lambda) >= delta:
            return fabs(lambda)
        else:
            return 0.5 * (lambda * lambda / delta + delta)

        This is the Roe analogue of a parameter-study knob.
        It is not the same as Project 2's artificial viscosity beta.
    */
}

static Flux roe_flux(solver *self, Conserved qL, Conserved qR)
{
    /*
        TODO(Project 3):
        Core Roe interface flux for Fhat_{i+1/2}.

        Input:
            qL = Q_i
            qR = Q_{i+1}

        Step 1: primitive states.

            WL = primitive_from_q(self, qL)
            WR = primitive_from_q(self, qR)

        Step 2: physical fluxes.

            FL = physical_flux_from_q(self, qL)
            FR = physical_flux_from_q(self, qR)

        Step 3: Roe averages.

            sL = sqrt(WL.rho)
            sR = sqrt(WR.rho)
            denom = sL + sR

            u_tilde = (sL * WL.u + sR * WR.u) / denom
            H_tilde = (sL * WL.H + sR * WR.H) / denom
            a2_tilde = (gamma - 1) * (H_tilde - 0.5 * u_tilde * u_tilde)
            a_tilde = sqrt(a2_tilde)
            rho_tilde = sL * sR

        Step 4: eigenvalues.

            lambda1 = u_tilde - a_tilde
            lambda2 = u_tilde
            lambda3 = u_tilde + a_tilde

            abs_l1 = entropy_fixed_abs_lambda(self, lambda1, a_tilde)
            abs_l2 = entropy_fixed_abs_lambda(self, lambda2, a_tilde)
            abs_l3 = entropy_fixed_abs_lambda(self, lambda3, a_tilde)

        Step 5: wave strengths.

            drho = WR.rho - WL.rho
            du   = WR.u   - WL.u
            dp   = WR.p   - WL.p

            alpha1 = 0.5 * (dp - rho_tilde * a_tilde * du) / a2_tilde
            alpha2 = drho - dp / a2_tilde
            alpha3 = 0.5 * (dp + rho_tilde * a_tilde * du) / a2_tilde

            Verify this alpha convention against your class notes or Toro.

        Step 6: right eigenvectors.

            r1 = [1, u_tilde - a_tilde, H_tilde - u_tilde * a_tilde]
            r2 = [1, u_tilde,           0.5 * u_tilde * u_tilde]
            r3 = [1, u_tilde + a_tilde, H_tilde + u_tilde * a_tilde]

        Step 7: assemble.

            Fhat = 0.5 * (FL + FR)
                 - 0.5 * (abs_l1 * alpha1 * r1
                        + abs_l2 * alpha2 * r2
                        + abs_l3 * alpha3 * r3)

        Return Fhat.
    */
}

int solver_allocate(solver *self)
{
    /*
        TODO(Project 3):
        Mirror Project 2's allocation discipline, but allocate Roe arrays:

            q1, q2, q3
            q1_next, q2_next, q3_next
            fhat1, fhat2, fhat3
            rho, u, p, E, a

        Not needed:
            q1_bar/q2_bar/q3_bar
            f1_bar/f2_bar/f3_bar
            visc_sensor

        Return 1 on success, 0 on failure.
    */
}

void solver_update_primitives(solver *self)
{
    /*
        TODO(Project 3):
        Same role as Project 2's update_primitives().

        for i = 0 .. nx-1:
            q = conserved_at(q1, q2, q3, i)
            W = primitive_from_q(q)
            rho[i] = W.rho
            u[i]   = W.u
            p[i]   = W.p
            E[i]   = W.E
            a[i]   = W.a if physical, otherwise 0
    */
}

double solver_compute_dt(solver *self)
{
    /*
        TODO(Project 3):
        Same CFL logic as Project 2.

        update_primitives()
        max_speed = max_i(fabs(u[i]) + a[i])
        if max_speed <= 0: return 1e-8
        return cfl * dx / max_speed

        Start with CFL = 0.5 for comparison with Project 2.
    */
}

void solver_apply_boundary(solver *self, double *q1, double *q2, double *q3)
{
    /*
        TODO(Project 3):
        Same simple transmissive boundary as Project 2:

            q[0]      = q[1]
            q[nx - 1] = q[nx - 2]

        This keeps the teaching code close to Project 2.
        Later, a cleaner finite-volume version can introduce explicit ghost cells.
    */
}

int solver_check_physical_state(
    solver *self,
    const double *q1,
    const double *q2,
    const double *q3
) {
    /*
        TODO(Project 3):
        This should be stricter than Project 2's commented physical clamp.

        for each cell:
            if q1[i] <= rho_floor: fail
            p = pressure_from_q(...)
            if p <= p_floor: fail
            if any q or p is NaN/Inf: fail

        Return:
            1 = physical enough to continue
            0 = unstable/nonphysical

        Recommended behavior:
            print case, step, time, cell index, rho, u, p
            self->failed = 1

        Do not silently force positive pressure in the first Roe baseline,
        because the report should know where Roe fails.
    */
}

void solver_init_riemann(solver *self)
{
    /*
        TODO(Project 3):
        Same role as Project 2's solver_init_sod(), but the name should reflect
        that configure_riemann_case() can choose any of the seven cases.

        for i = 0 .. nx-1:
            x = xmin + i * dx
            if x < x0:
                W = left state
            else:
                W = right state

            q1[i] = rho
            q2[i] = rho * u
            q3[i] = total_energy_density(self, rho, u, p)

        apply_boundary(q)
        check_physical_state(q)
        update_primitives()
    */
}

void solver_compute_all_roe_fluxes(solver *self)
{
    /*
        TODO(Project 3):
        Compute all interface fluxes.

        apply_boundary(q) should already have been called.

        for k = 0 .. nx-2:
            qL = conserved_at(q1, q2, q3, k)
            qR = conserved_at(q1, q2, q3, k + 1)
            Fhat = roe_flux(self, qL, qR)

            fhat1[k] = Fhat.f1
            fhat2[k] = Fhat.f2
            fhat3[k] = Fhat.f3

        Meaning:
            fhat[k] = Fhat_{k+1/2}
            cell i uses fhat[i] as right flux and fhat[i-1] as left flux.
    */
}

void solver_step_roe(solver *self)
{
    /*
        TODO(Project 3):
        Roe explicit finite-volume step.

        dt = compute_dt()
        if t + dt > t_max:
            dt = t_max - t
        lambda = dt / dx

        apply_boundary(q)
        compute_all_roe_fluxes()

        for i = 1 .. nx-2:
            q1_next[i] = q1[i] - lambda * (fhat1[i] - fhat1[i - 1])
            q2_next[i] = q2[i] - lambda * (fhat2[i] - fhat2[i - 1])
            q3_next[i] = q3[i] - lambda * (fhat3[i] - fhat3[i - 1])

        apply_boundary(q_next)

        if !check_physical_state(q_next):
            failed = 1
            return

        memcpy(q, q_next)
        t += dt
        step_count += 1
        update_primitives()

        Difference from Project 2:
            no predictor state
            no corrector state
            no artificial viscosity add-on in baseline
    */
}

void solver_write_tecplot(solver *self, const char *filename)
{
    /*
        TODO(Project 3):
        Preserve Project 2's variable contract exactly:

            TITLE = "1D Euler Roe Result"
            VARIABLES = "x", "rho", "u", "p", "E", "q1", "q2", "q3"
            ZONE T="t=...", I=nx, F=POINT

        This makes tecplot_compare.py, beta_sweep.py-derived metrics,
        parameter_matrix.py-derived metrics, and tecplot_animate.py easy to
        port or reuse.
    */
}

void solver_destroy(solver *self)
{
    /*
        TODO(Project 3):
        Free:
            q1, q2, q3
            q1_next, q2_next, q3_next
            fhat1, fhat2, fhat3
            rho, u, p, E, a

        Then:
            memset(self, 0, sizeof(*self))
    */
}

void solver_bind_methods(solver *self)
{
    /*
        TODO(Project 3):
        Bind exactly once in solver_create_default():

            allocate               -> solver_allocate
            bind_methods           -> solver_bind_methods
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
        Mirror Project 2 defaults first:

            nx = 501
            output_interval = 0
            domain = [0, 1]
            x0 = 0.5
            t = 0
            t_max = 0.2
            cfl = 0.5
            gamma = 1.4
            Sod left/right states
            rho_floor = 1e-10
            p_floor = 1e-10

        Roe-specific defaults:

            entropy_fix_type = ENTROPY_FIX_HARTEN
            entropy_fix_factor = 0.1
            entropy_fix_delta = 0.0
            failed = 0

        Then bind_methods().
    */
}

static int read_line(char *buffer, size_t size)
{
    /*
        TODO(Project 3):
        Same as Project 2.
        Use fgets, strip CR/LF, return 1 on success.
    */
}

static int ask_int_range(const char *prompt, int default_value, int min_value, int max_value)
{
    /*
        TODO(Project 3):
        Same input guard pattern as Project 2.
    */
}

static double ask_double_min(const char *prompt, double default_value, double min_value)
{
    /*
        TODO(Project 3):
        Same input guard pattern as Project 2.
    */
}

static int ask_yes_no(const char *prompt, int default_value)
{
    /*
        TODO(Project 3):
        Same y/n input pattern as Project 2.
    */
}

static void configure_riemann_case(solver *self, int *case_id)
{
    /*
        TODO(Project 3):
        Copy Project 2's seven cases exactly at first:

            1 Sod shock tube
            2 Lax shock tube
            3 subsonic double-expansion
            4 Sjogreen supersonic expansion
            5 contact discontinuity with double expansion
            6 contact discontinuity with double shock
            7 pure contact discontinuity

        Same case states and t_max values let Project 3 compare directly
        against Project 2 and Project 0 exact solutions.
    */
}

static void configure_domain_and_grid(solver *self)
{
    /*
        TODO(Project 3):
        Same as Project 2:

            ask xmin
            ask xmax
            ask x0
            guard x0 inside domain
            ask nx
            dx = (xmax - xmin) / (nx - 1)
    */
}

static void configure_numerics(solver *self)
{
    /*
        TODO(Project 3):
        Same Project 2 baseline prompts:

            gamma
            CFL
            t_max
            snapshot output interval

        Replace artificial-viscosity prompts with Roe prompts:

            enable entropy fix? y/n
            entropy fix factor, e.g. 0.1
            entropy fix absolute delta, e.g. 0.0

        This preserves the same interactive-driver strategy used by
        Project 2's beta_sweep.py and parameter_matrix.py.
    */
}

static void ask_output_filename(char *filename, size_t size, int case_id)
{
    /*
        TODO(Project 3):
        Same as Project 2 but use Roe-aware default:

            runs\\case_XX_roe_numerical.dat
    */
}

static void make_exact_filename(const char *numerical_filename, char *exact_filename, size_t size)
{
    /*
        TODO(Project 3):
        Same as Project 2:

            numerical.dat       -> numerical_exact.dat
            file.ext            -> file_exact.ext
            file_without_suffix -> file_without_suffix_exact.dat
    */
}

static void make_snapshot_filename(
    const char *final_filename,
    int step,
    char *snapshot_filename,
    size_t size
) {
    /*
        TODO(Project 3):
        Same as Project 2:

            case_01_roe_numerical.dat
            case_01_roe_numerical_step_000100.dat

        This keeps tecplot_animate.py easy to port.
    */
}

static int run_exact_solver(
    const solver *self,
    double output_time,
    const char *exact_filename
) {
    /*
        TODO(Project 3):
        Reuse Project 2's external exact-solver process contract.

        Command shape:

            riemann_exact.exe --batch
                left_rho left_u left_p
                right_rho right_u right_p
                gamma
                xmin xmax x0
                nx
                output_time
                exact_filename

        The exact solver is independent of MacCormack/Roe.
    */
}

static void print_banner(void)
{
    /*
        TODO(Project 3):
        Same role as Project 2, but label:

            1-D Euler Roe CFD Solver
    */
}

static void print_run_summary(solver *self, int case_id, const char *filename)
{
    /*
        TODO(Project 3):
        Same as Project 2 summary, but replace viscosity line with:

            entropy_fix = off / Harten
            entropy_factor = ...
            entropy_delta = ...

        Keep case/domain/grid/time/gamma/left/right/snapshots/output.
    */
}

int main(void)
{
    /*
        TODO(Project 3):
        Keep current Project 2 main-flow shape.

        solver my_solver = solver_create_default()

        print_banner()
        configure_riemann_case(&my_solver, &case_id)
        configure_domain_and_grid(&my_solver)
        configure_numerics(&my_solver)
        ask_output_filename(output_filename, ..., case_id)
        make_exact_filename(output_filename, exact_filename, ...)
        print_run_summary()

        if !allocate(): fail

        init_riemann()

        if output_interval > 0:
            write initial snapshot
            run exact solver at t = 0

        while t < t_max and step_count < 200000 and !failed:
            step_roe()
            if failed: break
            if snapshot interval hits:
                write numerical snapshot
                run exact solver at same t

        if failed:
            write diagnostic final state if still meaningful
            return nonzero or let driver mark unstable

        write final numerical file
        run exact solver at final t
        destroy()

        return 0

        Driver compatibility:
            Project 2's automation feeds interactive answers through stdin.
            Project 3 should keep the same ordering where possible, changing
            only the stabilizer prompts from viscosity beta/sensor to Roe
            entropy-fix settings.
    */
}
