#include <math.h>
#include <uWS/uWS.h>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include "Eigen-3.3/Eigen/Core"
#include "Eigen-3.3/Eigen/QR"
#include "MPC.h"
#include "json.hpp"

// for convenience
using json = nlohmann::json;

// For converting back and forth between radians and degrees.
constexpr double pi() { return M_PI; }
double deg2rad(double x) { return x * pi() / 180; }
double rad2deg(double x) { return x * 180 / pi(); }

// Checks if the SocketIO event has JSON data.
// If there is data the JSON object in string format will be returned,
// else the empty string "" will be returned.
string hasData(string s) {
	auto found_null = s.find("null");
	auto b1 = s.find_first_of("[");
	auto b2 = s.rfind("}]");
	if (found_null != string::npos) {
		return "";
	} else if (b1 != string::npos && b2 != string::npos) {
		return s.substr(b1, b2 - b1 + 2);
	}
	return "";
}

// Evaluate a polynomial.
double polyeval(Eigen::VectorXd coeffs, double x) {
	double result = 0.0;
	for (int i = 0; i < (int)	coeffs.size(); i++) {
		result += coeffs[i] * pow(x, i);
	}
	return result;
}

// Evaluate the derivative of a polynomial.
double polyeval_derivative(Eigen::VectorXd coeffs, double x) {
	double result = 0.0;
	for (int i = 1; i < (int)coeffs.size(); i++) {
		result += i * coeffs[i] * pow(x, i-1);
	}
	return result;
}
// Fit a polynomial.
// Adapted from
// https://github.com/JuliaMath/Polynomials.jl/blob/master/src/Polynomials.jl#L676-L716
Eigen::VectorXd polyfit(Eigen::VectorXd xvals, Eigen::VectorXd yvals,
                        int order)
{
	assert(xvals.size() == yvals.size());
	assert(order >= 1 && order <= xvals.size() - 1);
	Eigen::MatrixXd A(xvals.size(), order + 1);

	for (int i = 0; i < xvals.size(); i++) {
		A(i, 0) = 1.0;
	}

	for (int j = 0; j < xvals.size(); j++) {
		for (int i = 0; i < order; i++) {
			A(j, i + 1) = A(j, i) * xvals(j);
		}
	}

	auto Q = A.householderQr();
	auto result = Q.solve(yvals);
	return result;
}


void applyRotationAndTranslation( double x, double y, double px, double py, double psi, vector<double> &list_x, vector<double> &list_y)
{
	double dx = x - px;
	double dy = y - py;
	list_x.push_back(dx * cos(-psi) - dy * sin(-psi));
	list_y.push_back(dx * sin(-psi) + dy * cos(-psi));
}

int main() {
	uWS::Hub h;

	// MPC is initialized here!
	MPC mpc;

	h.onMessage([&mpc](	uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length,
						uWS::OpCode opCode) {
		// "42" at the start of the message means there's a websocket message event.
		// The 4 signifies a websocket message
		// The 2 signifies a websocket event
		string sdata = string(data).substr(0, length);
		cout << sdata << endl;
		if (sdata.size() > 2 && sdata[0] == '4' && sdata[1] == '2') {
			string s = hasData(sdata);
			if (s != "") {
				auto j = json::parse(s);
				string event = j[0].get<string>();
				if (event == "telemetry") {
					// j[1] is the data JSON object
					vector<double> ptsx = j[1]["ptsx"];
					vector<double> ptsy = j[1]["ptsy"];
					double px = j[1]["x"];
					double py = j[1]["y"];
					double psi = j[1]["psi"];
					double v = j[1]["speed"];
					double delta = j[1]["steering_angle"];
					double acceleration = j[1]["throttle"];


					// Calculate steering angle and throttle using MPC.
					// Get the points from the reference path and translate them to the car's perspective
					vector<double> reference_path_x;
					vector<double> reference_path_y;

					for (int ii = 0; ii < (int)ptsx.size(); ii++) {
						applyRotationAndTranslation( ptsx[ii], ptsy[ii], px, py, psi, reference_path_x, reference_path_y);
					}

					Eigen::Map<Eigen::VectorXd> reference_path_x_eig(&reference_path_x[0], 6);
					Eigen::Map<Eigen::VectorXd> reference_path_y_eig(&reference_path_y[0], 6);

					auto coeffs = polyfit( reference_path_x_eig, reference_path_y_eig , 3);

					double cte  = polyeval(coeffs, 0);
					double epsi = -atan( polyeval_derivative(coeffs, 0) );

					Eigen::VectorXd state(6);

					// predict state in 100ms
					const double Lf = 2.67;
					double latency = 0.1;
					px = 0.0 + v*latency;
					py = 0.0;
					psi = 0.0 + v*(-delta)/Lf*latency;
					v = v + acceleration*latency;
			        cte = cte + v * sin(epsi) * latency;
			        epsi = epsi + v * (-delta) / Lf * latency;

			        state << px, py, psi, v, cte, epsi;
					auto vars = mpc.Solve(state, coeffs);

					double steer_value = vars[0];
					double throttle_value = vars[1];

					json msgJson;
					// NOTE: Remember to divide by deg2rad(25) before you send the steering value back.
					// Otherwise the values will be in between [-deg2rad(25), deg2rad(25] instead of [-1, 1].
		            msgJson["steering_angle"] = -steer_value/deg2rad(25);
		            msgJson["throttle"] = throttle_value;

					//Display the MPC predicted trajectory
					vector<double> mpc_x_vals;
					vector<double> mpc_y_vals;

					//.. add (x,y) points to list here, points are in reference to the vehicle's coordinate system
					// the points in the simulator are connected by a Green line
					for (int ii = 2; ii < (int)vars.size(); ++ii) {
						if (ii % 2 == 0)
							mpc_x_vals.push_back( vars[ii] );
						else
							mpc_y_vals.push_back( vars[ii] );
					}

					msgJson["mpc_x"] = mpc_x_vals;
					msgJson["mpc_y"] = mpc_y_vals;

					//Display the reference_path/reference line
					vector<double> next_x_vals;
					vector<double> next_y_vals;

					//.. add (x,y) points to list here, points are in reference to the vehicle's coordinate system
					// the points in the simulator are connected by a Yellow line
					for(int ii = 0; ii < 10 ; ++ii){
						double aux_x, aux_y;
						aux_x = (double) ii * 5.0;
						aux_y = polyeval( coeffs, aux_x );
						next_x_vals.push_back( aux_x );
						next_y_vals.push_back( aux_y );
					}

					msgJson["next_x"] = next_x_vals;
					msgJson["next_y"] = next_y_vals;


					auto msg = "42[\"steer\"," + msgJson.dump() + "]";
					std::cout << msg << std::endl;

					// Latency
					this_thread::sleep_for(chrono::milliseconds(100));
					ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
				}
			} else {
				// Manual driving
				std::string msg = "42[\"manual\",{}]";
				ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
			}
		}
	});

	// We don't need this since we're not using HTTP but if it's removed the
	// program
	// doesn't compile :-(
	h.onHttpRequest([](uWS::HttpResponse *res, uWS::HttpRequest req, char *data,
						size_t, size_t) {
		const std::string s = "<h1>Hello world!</h1>";
		if (req.getUrl().valueLength == 1) {
			res->end(s.data(), s.length());
		} else {
			// i guess this should be done more gracefully?
			res->end(nullptr, 0);
		}
	});

	h.onConnection([&h](uWS::WebSocket<uWS::SERVER> ws, uWS::HttpRequest req) {
		std::cout << "Connected!!!" << std::endl;
	});

	h.onDisconnection([&h](uWS::WebSocket<uWS::SERVER> ws, int code,
							char *message, size_t length) {
		ws.close();
		std::cout << "Disconnected" << std::endl;
	});

	int port = 4567;
	if (h.listen(port)) {
		std::cout << "Listening to port " << port << std::endl;
	} else {
		std::cerr << "Failed to listen to port" << std::endl;
		return -1;
	}
	h.run();
}
