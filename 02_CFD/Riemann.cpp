#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <iomanip>

using namespace std;

// 气体状态
struct State
{
    double rho; 
    double u;   
    double p;   
};

class ExactRiemannSolver
{
private:
    double gamma;//比热比
    double tol; //允许的压力下限
    int max_iter;//Newton迭代最大次数

    // 计算声速
    double sound_speed(double rho, double p)
    {
        return sqrt(gamma * p / rho);
    }

    // 函数 f(p, W_K) 及导数 f'(p, W_K)，用于Newton迭代,即书上的公式3.6.17，Wk为初始状态
    void eval_f_fd(double p, double rho_k, double p_k, double c_k, double& f, double& fd)//传入引用以修改值
    {
        if (p <= p_k)
        {
            // 膨胀波，等熵膨胀，使用等熵关系式计算函数值和导数
            double p_ratio = p / p_k;
            double p_pow = pow(p_ratio, (gamma - 1.0) / (2.0 * gamma));
            f = 2.0 * c_k / (gamma - 1.0) * (p_pow - 1.0);
            fd = (1.0 / (rho_k * c_k)) * pow(p_ratio, -(gamma + 1.0) / (2.0 * gamma));
        }
        else
        {
            // 激波，使用Rankine-Hugoniot条件计算函数值和导数
            double A = 2.0 / ((gamma + 1.0) * rho_k);
            double B = (gamma - 1.0) / (gamma + 1.0) * p_k;
            double sqrt_term = sqrt(A / (p + B));
            f = (p - p_k) * sqrt_term;
            fd = (1.0 - 0.5 * (p - p_k) / (p + B)) * sqrt_term;
        }
    }

    // 迭代求解中间区压力 p*，即找到一个 p*，使得左侧产生的速度增量和右侧产生的速度增量在接触间断处匹配
    double solve_p_star(State L, State R, double cL, double cR) 
    {
        // 用算术平均 0.5 * (L.p + R.p) 作为迭代的初始值
        double p_star = 0.5 * (L.p + R.p);
        double dp = 1.0;
        int iter = 0;
        //利用 eval_f_fd 计算当前的函数值 f 和导数 fd，并更新压力：p_n+1 = p_n - f / f'
        while (abs(dp) > tol && iter < max_iter)
        {
            double fL, fdL, fR, fdR;
            eval_f_fd(p_star, L.rho, L.p, cL, fL, fdL);
            eval_f_fd(p_star, R.rho, R.p, cR, fR, fdR);

            double f = fL + fR + R.u - L.u;//公式3.6.18
            double fd = fdL + fdR;

            dp = f / fd;
            p_star -= dp;

            // 限制压力为正
            if (p_star < tol) p_star = tol;
            iter++;
        }
        return p_star;
    }

public:
    ExactRiemannSolver(double gamma_val = 1.4, double tolerance = 1e-6, int iterations = 100)//默认参数
        : gamma(gamma_val), tol(tolerance), max_iter(iterations) {}

    // 给定相似变量 S = x/t（即波的传播速度），判断该位置位于哪种波系结构中，并计算出局部的rho, u, p。
    State sample(State L, State R, double p_star, double u_star, double S)
    {
        State out;
        double cL = sound_speed(L.rho, L.p);
        double cR = sound_speed(R.rho, R.p);

        //首先通过接触间断划分左右，如果 S <= u*，说明采样点在接触间断左侧,反之在右侧

        /*
        以左侧 S <= u*) 为例，
        比较 p* 和 p_L。如果 p* <= p_L，说明产生的是左膨胀波。膨胀波是一个扇形区域，
        需要进一步判断S是在未扰动区（波头左侧）、膨胀区内部（需要线性插值计算），还是中间区。

        如果 p* > p_L，说明产生的是左激波。
        激波是一条间断线，只需判断 S 是在激波前还是激波后，使用激波波速进行判断。
        */
        if (S <= u_star) 
        {
            // 左侧波
            if (p_star <= L.p) 
            {
                // 左膨胀波
                double SHL = L.u - cL;
                double cL_star = cL * pow(p_star / L.p, (gamma - 1.0) / (2.0 * gamma));
                double STL = u_star - cL_star;

                if (S <= SHL)
                {
                    out = L;
                }
                else if (S > SHL && S < STL) 
                { // 膨胀波扇区内
                    out.u = 2.0 / (gamma + 1.0) * (cL + (gamma - 1.0) / 2.0 * L.u + S);
                    double c = 2.0 / (gamma + 1.0) * (cL + (gamma - 1.0) / 2.0 * (L.u - S));
                    out.rho = L.rho * pow(c / cL, 2.0 / (gamma - 1.0));
                    out.p = L.p * pow(c / cL, 2.0 * gamma / (gamma - 1.0));
                }
                else 
                { // 中间区左侧
                    out.p = p_star;
                    out.u = u_star;
                    out.rho = L.rho * pow(p_star / L.p, 1.0 / gamma);
                }
            }
            else
            {
                // 左激波
                double p_ratio = p_star / L.p;
                double SL = L.u - cL * sqrt((gamma + 1.0) / (2.0 * gamma) * p_ratio + (gamma - 1.0) / (2.0 * gamma));
                if (S <= SL) 
                {
                    out = L;
                }
                else
                { // 中间区左侧
                    out.p = p_star;
                    out.u = u_star;
                    out.rho = L.rho * (p_ratio + (gamma - 1.0) / (gamma + 1.0)) / (p_ratio * (gamma - 1.0) / (gamma + 1.0) + 1.0);
                }
            }
        }
        else 
        {
            // 右侧波
            if (p_star > R.p) 
            {
                // 右激波
                double p_ratio = p_star / R.p;
                double SR = R.u + cR * sqrt((gamma + 1.0) / (2.0 * gamma) * p_ratio + (gamma - 1.0) / (2.0 * gamma));
                if (S >= SR) 
                {
                    out = R;
                }
                else 
                { // 中间区右侧
                    out.p = p_star;
                    out.u = u_star;
                    out.rho = R.rho * (p_ratio + (gamma - 1.0) / (gamma + 1.0)) / (p_ratio * (gamma - 1.0) / (gamma + 1.0) + 1.0);
                }
            }
            else 
            {
                // 右膨胀波
                double SHR = R.u + cR;
                double cR_star = cR * pow(p_star / R.p, (gamma - 1.0) / (2.0 * gamma));
                double STR = u_star + cR_star;

                if (S >= SHR)
                {
                    out = R;
                }
                else if (S < SHR && S > STR) 
                { // 膨胀波扇区内
                    out.u = 2.0 / (gamma + 1.0) * (-cR + (gamma - 1.0) / 2.0 * R.u + S);
                    double c = 2.0 / (gamma + 1.0) * (cR - (gamma - 1.0) / 2.0 * (R.u - S));
                    out.rho = R.rho * pow(c / cR, 2.0 / (gamma - 1.0));
                    out.p = R.p * pow(c / cR, 2.0 * gamma / (gamma - 1.0));
                }
                else
                { // 中间区右侧
                    out.p = p_star;
                    out.u = u_star;
                    out.rho = R.rho * pow(p_star / R.p, 1.0 / gamma);
                }
            }
        }
        return out;
    }

   
    // 求解并输出到Tecplot
    void solve(State L, State R, double t, double x_min, double x_max, int nx, const string& filename) {
        double cL = sound_speed(L.rho, L.p);
        double cR = sound_speed(R.rho, R.p);

        // 1. 计算中间区状态
        double p_star = solve_p_star(L, R, cL, cR);

        double fL, fdL;
        eval_f_fd(p_star, L.rho, L.p, cL, fL, fdL);
        double u_star = L.u - fL;

        // 2. 空间网格
        double dx = (x_max - x_min) / (nx - 1);

        ofstream outfile(filename);//从圆柱绕流把Tecplot的文件格式拿过来
        outfile << "TITLE = \"1D Exact Riemann Solution\"\n";
        outfile << "VARIABLES = \"x\", \"rho\", \"u\", \"p\", \"Mach\"\n";// 定义变量名称
        outfile << "ZONE T=\"t=" << t << "\", I=" << nx << ", F=POINT\n";// 定义 Zone：I 为一维节点数，F=POINT 表示按行排列数据点，T 标记时间步

        for (int i = 0; i < nx; i++) 
        {
            double x_pos = x_min + i * dx;
            double S = x_pos / t; // 相似变量 x/t
            State res = sample(L, R, p_star, u_star, S);

            // 马赫数
            double c = sound_speed(res.rho, res.p);
            double Mach = std::abs(res.u) / c;

           
            outfile << fixed << setprecision(6)
                << x_pos << "\t"
                << res.rho << "\t"
                << res.u << "\t"
                << res.p << "\t"
                << Mach << "\n";
        }
        outfile.close();
        cout << "Calculation finished.Result is output to: " << filename << endl;
    }
};

int main() 
{
    // Lax
    State LeftState = { 0.4, 0.5, 1.0 };
    State RightState = { 1.0, -0.5, 0.9 };

    double t = 3.5;         // 时间
    double x_min = -20.0;    // 空间左边界
    double x_max = 20.0;     // 空间右边界
    int nx = 500;           // 空间点数

    ExactRiemannSolver solver(1.4); // gamma = 1.4
    solver.solve(LeftState, RightState, t, x_min, x_max, nx, "riemann_solution.plt");

    return 0;
}