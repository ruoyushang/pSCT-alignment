//
// Created by Deivid Ribeiro on 5/11/21.
//
#include "stewartplatform.hpp"
#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include "AGeoAsphericDisk.h" // ROBAST dependency
#include "TMinuit.h" // ROOT's implementation of MINUIT for chiSq minimization

#include "client/controllers/mirrorcontroller.hpp"

#include "common/alignment/device.hpp"
#include "common/simulatestewart/mathtools.hpp"
#include "common/simulatestewart/mirrordefinitions.hpp" // definitions of the mirror surfaces

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

class Mirror {
public:
	explicit Mirror(Device::Identity identity);

	// initialize and precompute everything
	bool initialize();

	// COORDINATE TRANSFORMATION HELPERS
	// reference frame tansformations:
	double _getAzOffset(unsigned pos);

	static Eigen::Matrix3d _rotMat(int axis, double a);

	// to panel reference frame (from telescope). Order of rotations -- z -> x-> y
	Eigen::Vector3d _toPanelRF(unsigned pos, const Eigen::Vector3d &in_coords);

	// to telescope reference frame (from panel)
	Eigen::Vector3d _toTelRF(unsigned pos, const Eigen::Vector3d &in_coords);

	// move a vector by in_coords in the current reference frame.
	// The current reference frame is the frame in which in_vec coords and tr_coords
	// transformations are given.
	// the input is a 3D vector and the 6 coords by which to transform,
	// tr_coords -- x, y, z, xRot, yRot, zRot
	// the output is the transformed 3D vector
	Eigen::Vector3d __moveInCurrentRF(const Eigen::Vector3d &in_vec, const Eigen::VectorXd &tr_coords);

private:
	Device::Identity m_Identity;

	// A simulated surface of this mirror
	std::shared_ptr<AGeoAsphericDisk> m_pSurface;
	// A simulated stewart platform
	std::shared_ptr<StewartPlatform> m_pStewartPlatform;

	// mirror coords -- x/y/z, xRot, yRot, zRot
	Eigen::VectorXd m_curCoords, m_curCoordsErr, m_sysOffsetsMPES;
	// map ring -> {MPES Position -> Systematic Offset}
	std::map<unsigned, std::map<unsigned, Eigen::VectorXd> > SystematicOffsetsMPESMap;

	// in our coordinate system, the ideal panel is located along the x axis, between
	// the second and third quadrant
	// we keep Azimuthal offset of x1x1 from this ideal panel
	std::map<unsigned, double> m_PanelOffset;

	// angular panel width in radians
	// ring -> {panel width}
	std::map<unsigned, double> m_PanelWidth;

	// precomputed ideal panel origin in the telescope frame -- both the inner and outer rings
	// ring->{location vector}
	// currently setting to zeros and populating in initialize()
	std::map<unsigned, Eigen::Vector3d> m_PanelOriginTelFrame;

	// pad positions in the telescope frame for both rings -
	// convenience copy of SCT::<Mirror>::PadsCoords. Pad per column of the matrix
	std::map<unsigned, Eigen::Matrix3d> m_PadCoordsTelFrame;

	// precomputed ideal panel reference frame (basis vectors) in the telescope frame,
	// both the inner and outer rings. This to facilitate coordinate transformations
	// ring->{Column Matrix}
	// Populated in initialize()
	std::map<unsigned, Eigen::Matrix3d> m_PanelFrame;

	// the direction of the norm of the whole mirror: +1 for Primary, -1 for secondary
	double m_SurfaceNorm = 1.;
};

const std::string SAVEFILE_DELIMITER = "****************************************";

Mirror::Mirror(Device::Identity identity) : m_Identity(std::move(identity)), m_pSurface(nullptr) {

	// define coordinate vectors -- these are of size 6
	m_curCoords = Eigen::VectorXd(6);
	m_curCoordsErr = Eigen::VectorXd(6);
	m_sysOffsetsMPES = Eigen::VectorXd(6);
	Eigen::VectorXd tmp = Eigen::VectorXd(2);
	tmp.setZero();
	// initialize the systematic offsets map to zeros
	SystematicOffsetsMPESMap = {
			{1, {
					    {1, tmp}, {2, tmp}, {3, tmp}, {4, tmp}, {5, tmp}
			    }},
			{2, {
					    {1, tmp}, {2, tmp}, {3, tmp}, {4, tmp}
			    }}
	};
}

bool Mirror::initialize() {
	spdlog::info("{} : Mirror::initialize(): Initializing...", m_Identity);

	// precompute everything we need
	if (m_Identity.position == 1) {
		// bottom (w.r.t. z) surface first, then top surface --
		// back surface first, then optical surface
		m_pSurface = std::make_shared<AGeoAsphericDisk>(m_Identity.name.c_str(),
		                                                SCT::Primary::kZ + SCT::Primary::kz[0] -
		                                                SCT::Primary::kMirrorThickness, 0,
		                                                SCT::Primary::kZ + SCT::Primary::kz[0], 0,
		                                                SCT::Primary::kD / 2., 0);
		m_pSurface->SetPolynomials(SCT::kNPar - 1, &SCT::Primary::kz[1],
		                           SCT::kNPar - 1, &SCT::Primary::kz[1]);
		m_SurfaceNorm = 1.; // along z

		// update ideal panel properties
		for (int ring : {0, 1}) {
			// ideal panel width and offset of x1x1 from it
			m_PanelWidth[ring + 1] = 2. * M_PI / SCT::Primary::kPanels[ring];

			for (int coord = 0; coord < 3; coord++)
				for (int pad = 0; pad < 3; pad++)
					m_PadCoordsTelFrame[ring + 1].col(pad)(coord) = SCT::Primary::PadsCoords[ring][pad][coord];

			m_PanelOriginTelFrame[ring + 1].setZero();
			// this is the center of the mirror panel in the TRF
			for (int pad = 0; pad < 3; pad++)
				m_PanelOriginTelFrame.at(ring + 1) += m_PadCoordsTelFrame.at(ring + 1).col(pad) / 3.;
		}

	} else if (m_Identity.position == 2) {
		// bottom (w.r.t. z) surface first, then top surface --
		// optical surface first, then back surface
		m_pSurface = std::make_shared<AGeoAsphericDisk>(m_Identity.name.c_str(),
		                                                SCT::Secondary::kZ + SCT::Secondary::kz[0] -
		                                                SCT::Secondary::kMirrorThickness,
		                                                0,
		                                                SCT::Secondary::kZ + SCT::Secondary::kz[0], 0,
		                                                SCT::Secondary::kD / 2., 0);
		m_pSurface->SetPolynomials(SCT::kNPar - 1, &SCT::Secondary::kz[1],
		                           SCT::kNPar - 1, &SCT::Secondary::kz[1]);
		m_SurfaceNorm = 1.; // parallel to z in its frame

		// update ideal panel properties
		for (int ring : {0, 1}) {
			// ideal panel width and offset of x1x1 from it
			m_PanelWidth[ring + 1] = 2. * M_PI / SCT::Secondary::kPanels[ring];

			// update pad coords and panel origin
			for (int coord = 0; coord < 3; coord++)
				for (int pad = 0; pad < 3; pad++)
					m_PadCoordsTelFrame[ring + 1].col(pad)(coord) = SCT::Secondary::PadsCoords[ring][pad][coord];

			m_PanelOriginTelFrame[ring + 1].setZero();
			// this is the center of the mirror panel in the TRF
			for (int pad = 0; pad < 3; pad++)
				m_PanelOriginTelFrame.at(ring + 1) += m_PadCoordsTelFrame.at(ring + 1).col(pad) / 3.;
		}
	} else if (m_Identity.position ==
	           3) { // this is used for the test setup in the lab (2 P2 panels) - duplicates the primary mirror geometry
		// bottom (w.r.t. z) surface first, then top surface --
		// back surface first, then optical surface
		m_pSurface = std::make_shared<AGeoAsphericDisk>(m_Identity.name.c_str(),
		                                                SCT::Primary::kZ + SCT::Primary::kz[0] -
		                                                SCT::Primary::kMirrorThickness, 0,
		                                                SCT::Primary::kZ + SCT::Primary::kz[0], 0,
		                                                SCT::Primary::kD / 2., 0);
		m_pSurface->SetPolynomials(SCT::kNPar - 1, &SCT::Primary::kz[1],
		                           SCT::kNPar - 1, &SCT::Primary::kz[1]);
		m_SurfaceNorm = 1.; // along z

		// update ideal panel properties
		for (int ring : {0, 1}) {
			// ideal panel width and offset of x1x1 from it
			m_PanelWidth[ring + 1] = 2. * M_PI / SCT::Primary::kPanels[ring];

			for (int coord = 0; coord < 3; coord++)
				for (int pad = 0; pad < 3; pad++)
					m_PadCoordsTelFrame[ring + 1].col(pad)(coord) = SCT::Primary::PadsCoords[ring][pad][coord];

			m_PanelOriginTelFrame[ring + 1].setZero();
			// this is the center of the mirror panel in the TRF
			for (int pad = 0; pad < 3; pad++)
				m_PanelOriginTelFrame.at(ring + 1) += m_PadCoordsTelFrame.at(ring + 1).col(pad) / 3.;
		}

	} else {
		spdlog::error("{} : Invalid mirror position {}. Could not initialize...", m_Identity, m_Identity.position);
		return false;
	}

	m_pStewartPlatform = std::make_shared<StewartPlatform>();

	// precompute ideal offsets and norms
	if (m_pSurface) {
		double dir[3] = {0., 0., m_SurfaceNorm}; // norm should point along this direction
		double norm[3];
		for (int ring : {0, 1}) {
			// set panel offset -- note the sign here
			m_PanelOffset[ring + 1] = M_PI - m_PanelWidth.at(ring + 1) / 2.;

			// get the norm
			m_pSurface->ComputeNormal(m_PanelOriginTelFrame.at(ring + 1).data(), dir, norm);
			// save the norm in the third column
			m_PanelFrame[ring + 1].col(2) = Eigen::Vector3d(norm);
			// make sure this is pointing where we need it to
			if (m_PanelFrame.at(ring + 1).col(2).dot(Eigen::Vector3d(dir)) < 0.)
				m_PanelFrame.at(ring + 1).col(2) *= -1;

			// get the x and y axes:
			for (int i = 0; i < 2; i++) {
				Eigen::Vector3d axis{0., 0., 0};
				axis(i) = 1.0;
				// subtract the projection onto the norm
				axis -= axis.dot(m_PanelFrame.at(ring + 1).col(2)) * m_PanelFrame.at(ring + 1).col(2);
				axis.normalize();
				m_PanelFrame.at(ring + 1).col(i) = axis;
			}

			// the true panel origin is at the base triangle, not at the mirror surface --
			// need to shift the one computed above to the base triangle
			// compute the center of the mirror panel in the panel frame:
			m_pStewartPlatform->SetPanelType(StewartPlatform::PanelType::OPT);
			double actL[6];
			for (auto &len : actL)
				len = SCT::kActuatorLength;
			m_pStewartPlatform->ComputeStewart(actL);
			Eigen::Vector3d PanelCenterPanelFrame(m_pStewartPlatform->GetPanelCoords());
			// this is the center of the mirror panel in the panel frame;
			// what we obtained before was the center of the mirror panel in the TRF --
			// need to shift those coordinates by this much opposite to the mirror norm
			// to get the origin of the panel frame in the TRF:
			m_PanelOriginTelFrame.at(ring + 1) -= m_PanelFrame.at(ring + 1) * PanelCenterPanelFrame;
		}
	}

	spdlog::info("{}: Mirror::initialize(): Done.", m_Identity);

	return true;
}

/* =========== Coordinate Transformations =========== */

// get the angular offset of this panel from our ideal one for which everything is precomputed
double Mirror::_getAzOffset(unsigned pos) {
	// we have the ideal origin and norm of the "nominal" panel.
	// Need to identify the current panel placement and transform accordingly
	//
	// pos = <MIRROR><QUADRANT><RING><PANEL>
	// we need the quadrant and the panel within it to calculate the azimuthal rotation
	int quadrant = SCTMath::Quadrant(pos); // 4 quadrants in total
	int ring = SCTMath::Ring(pos);
	int panel = SCTMath::Panel(pos);

	// number of panels in a quadrant
	double quadPanels = M_PI / 2. / m_PanelWidth.at(ring);
	// azimuthal offset of this panel from the ideal one
	// (numbering of the panels increases in the direction opposite from z rotation, hence the
	// minus sign below)
	double phiOffset = m_PanelOffset.at(ring) -
	                   (quadPanels * (quadrant - 1) + panel - 1) * m_PanelWidth.at(ring);

	return phiOffset;
}

Eigen::Vector3d Mirror::_toPanelRF(unsigned pos, const Eigen::Vector3d &in_coords) {
	// all angles in radians

	int ring = SCTMath::Ring(pos);
	double phi = _getAzOffset(pos);

	// this panel's frame is the rotated frame of the ideal panel
	Eigen::Matrix3d zRot = _rotMat(2, phi);
	Eigen::Matrix3d panelFrame = zRot * m_PanelFrame.at(ring);
	Eigen::Vector3d panelOrigin = zRot * m_PanelOriginTelFrame.at(ring);

	return panelFrame.transpose() * (in_coords - panelOrigin);
}

Eigen::Vector3d Mirror::_toTelRF(unsigned pos, const Eigen::Vector3d &in_coords) {
	// all angles in radians
	// The inverse of the above
	int ring = SCTMath::Ring(pos);
	double phi = _getAzOffset(pos);

	// this panel's frame is the rotated frame of the ideal panel
	Eigen::Matrix3d zRot = _rotMat(2, phi);
	Eigen::Matrix3d panelFrame = zRot * m_PanelFrame.at(ring);
	Eigen::Vector3d panelOrigin = zRot * m_PanelOriginTelFrame.at(ring);

	// remember panel frame is an orthogonal matrix -- very simple inversion
	return panelFrame * in_coords + panelOrigin;
}

Eigen::Matrix3d Mirror::_rotMat(int axis, double a) {
	double c = cos(a);
	double s = sin(a);

	Eigen::Matrix3d rot;
	if (axis == 0) // x rotation
		rot << 1., 0., 0.,
				0., c, -s,
				0., s, c;
	else if (axis == 1) // y rotation
		rot << c, 0., s,
				0., 1., 0.,
				-s, 0., c;
	else if (axis == 2) // z rotation
		rot << c, -s, 0.,
				s, c, 0.,
				0., 0., 1.;

	return rot;
}

Eigen::Vector3d Mirror::__moveInCurrentRF(const Eigen::Vector3d &in_vec, const Eigen::VectorXd &tr_coords) {
	// check that we have 6 coords. if not, resize if necessary
	Eigen::VectorXd tr(6);
	tr.head(tr_coords.size()) = tr_coords;
	if (tr_coords.size() != 6)
		tr.tail(6 - tr_coords.size()).setZero();

	Eigen::Vector3d out_vec = in_vec;
	// compute the transform due to the change in TRF: Rot(z -> x -> y)*v + T
	out_vec = _rotMat(2, tr(5)) * out_vec;
	out_vec = _rotMat(0, tr(3)) * out_vec;
	out_vec = _rotMat(1, tr(4)) * out_vec;
	out_vec += tr.head(3);

	return out_vec;
}

int main(int argc, char *argv[]) {
	std::string usage = "<ACT file>";

	if (argc == 1) {
		std::cout << "Usage: " << argv[0] << " " << usage << std::endl;
		return -1;
	}

	std::vector<std::string> ACTpaths;
	for (int i = optind; i < argc; i++) {
		ACTpaths.push_back(argv[i]);
	}

	std::string loadFilePath;
	std::string saveFilePath;
	Device::Identity m_Identity;
	unsigned positionNum;

	for (const auto &path : ACTpaths) {
		loadFilePath = path;
		saveFilePath = path + "_physicalPosTelRF";

		//Check if file already exists
		struct stat buf{};
		if (stat(loadFilePath.c_str(), &buf) == -1) {
			spdlog::error("File {} not found. Please make sure the selected file path is valid.",
			              loadFilePath);
			return 0;
		}

		std::ostringstream os;

		// Open file stream
		std::ifstream infile(loadFilePath);
		if (infile.bad()) {
			spdlog::error("File {} cannot be read. Please check it and try again.", loadFilePath);
			return 0;
		}

		// Check to make sure it matches this mirror
		std::string line;
		getline(infile, line);
		unsigned s = line.find("(");
		unsigned e = line.find(")");
		Device::Identity mirrorId = Device::parseIdentity(line.substr(s, e - s + 2));
		m_Identity = mirrorId;
		Mirror mirror = Mirror(m_Identity);
		mirror.initialize();

		// Print Mirror Info
		std::map<Device::Identity, Eigen::VectorXd> panelPositions;
		while (getline(infile, line) && (line != SAVEFILE_DELIMITER)) {
			os << line << std::endl;
		}
		spdlog::info("Mirror Info:\n{}", os.str());

		Eigen::VectorXd ActLengthsFromFile(6);
		Eigen::VectorXd physicalCoords(6);
//        unsigned j = 0;

		// Parse all target actuator lengths
		Device::Identity panelId;
		int i = 0;
		StewartPlatform SP;
		// pad coords -- column per pad
		Eigen::Matrix3d m_PadCoords;

		// Will save all panel positions, including OT, if it is a child of this mirror.
		spdlog::info("{}: Attempting to write Mirror position to file {}...", m_Identity, saveFilePath);

		//Check if file already exists
		struct stat buf1{};
		if (stat(saveFilePath.c_str(), &buf1) != -1) {
			spdlog::error(
					"File {} already exists. Please select a different path, or manually delete/move/rename the file in your system.",
					saveFilePath);
			return OpcUa_Bad;
		}

		// Create output file stream
		std::ofstream outfile(saveFilePath, std::ofstream::out);

		if (outfile.bad()) {
			spdlog::error("Cannot write to file at {}. Aborting...", saveFilePath);
			outfile.close();
			return OpcUa_Bad;
		}

		// Place mirror name/Type and
		// other information at top of file
		outfile << "Mirror: " << m_Identity << std::endl;
		std::time_t now = std::time(nullptr);
		outfile << "Timestamp: " << std::ctime(&now) << std::endl;
		outfile << "Converted from original file " << loadFilePath << std::endl;
		outfile << SAVEFILE_DELIMITER << std::endl;

		while (infile.peek() != EOF) {
			i = 0;
			getline(infile, line);
			std::size_t found = line.find("Az:");
			if (found != std::string::npos) {
				continue;
			}
			s = line.find('(');
			e = line.find(')');
			panelId = Device::parseIdentity(line.substr(s, e - s + 2));

			while (getline(infile, line) && line != SAVEFILE_DELIMITER) {
				ActLengthsFromFile(i) = std::stod(line);
				i++;
			}

			panelPositions[panelId] = ActLengthsFromFile;
			spdlog::info("Found position for Panel {}:\n{}\n", panelId, ActLengthsFromFile);

			positionNum = panelId.position;
			if (SCTMath::Ring(positionNum) == 0) {
				continue;
			}

			SP.SetPanelType(StewartPlatform::PanelType::OPT);
			// update current coordinates
			SP.ComputeStewart(ActLengthsFromFile.data());
			// panel coords
			for (int j = 0; j < 6; j++) {
				physicalCoords[j] = SP.GetPanelCoords()[j];
			}

			// pad coords -- each column corresponds to a pad
			for (int pad = 0; pad < 3; pad++) {
				// populate panel frame pad coordinates
				for (int coord = 0; coord < 3; coord++) {
					m_PadCoords.col(pad)(coord) = SP.GetPadCoords(pad)[coord];
				}
			}

			// for this panel, we get PRF pad coords, transform them to TRF,
			// move them in TRF, transform back to PRF, and then compute new ACT lengths
			// based on the new pad coords. so simple!
			auto padCoords_PanelRF = m_PadCoords;
			auto padCoords_TelRF = padCoords_PanelRF;
			for (unsigned pad = 0; pad < 3; pad++) {
				// transform to TRF
				padCoords_TelRF.col(pad) = mirror._toTelRF(positionNum, padCoords_PanelRF.col(pad));
			}
			spdlog::info(padCoords_TelRF);

			outfile << "Panel: " << panelId << std::endl;
			outfile << padCoords_TelRF << std::endl;
			outfile << SAVEFILE_DELIMITER << std::endl;
			spdlog::info("moving on...");
		}

		spdlog::info("Saving...");
		outfile.close();
		spdlog::info("{}: Done writing Mirror position to file {}.", m_Identity, saveFilePath);
	}


}
