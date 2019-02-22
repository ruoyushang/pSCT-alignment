#include "AravisCamera.h"
using std::string;
using std::vector;


AravisCamera::AravisCamera(const char* device_id) :
	cameraIsReady(false), imgWidth(2592), imgHeight(1944), nBuffers(10) {

	// initialize glib
	arv_g_type_init();

	// initialize camera
	camera = arv_camera_new(device_id);
	
	if(camera != NULL) {
		// set camera parameters
		cameraID = arv_camera_get_device_id(camera);
		arv_camera_set_region(camera, 0, 0, imgWidth, imgHeight);
		arv_camera_set_binning(camera, 0, 0);    // don't bin image
		
		// initialize stream
		stream = arv_camera_create_stream(camera, NULL, NULL);

		if(stream != NULL) {
			payload = arv_camera_get_payload(camera);
			for(int i = 0; i < nBuffers; i++)
				arv_stream_push_buffer(stream, arv_buffer_new(payload, NULL));
			arv_camera_set_acquisition_mode(camera, ARV_ACQUISITION_MODE_CONTINUOUS);
			arv_camera_set_trigger(camera, "Software");

			// enable acquisition
			arv_camera_start_acquisition(camera);
			sleep(1);
			cameraIsReady = true;
		}
	}
}

AravisCamera::~AravisCamera() {
	cameraIsReady = false;
    if(camera != NULL && stream != NULL) {
        arv_camera_stop_acquisition(camera);
        g_object_unref(stream);    // free stream and buffers
        g_object_unref(camera);    // free camera
        stream = nullptr;
        camera = nullptr;
    }
}

bool AravisCamera::isReady() {
	return cameraIsReady;
}

vector<unsigned char> AravisCamera::captureFrame() {
	// create vector which will hold image
	vector<unsigned char> image;
	image.reserve(payload);

	// send a trigger to the camera
	arv_camera_software_trigger(camera);
	
	// try buffer retrievel or timeout after 2 second
	ArvBuffer* buffer = arv_stream_timeout_pop_buffer(stream, 2000000);
	
	if(buffer == NULL)
		std::cout << "IMAGE BUFFER IS NULL" << std::endl;

	/*
	 * ArvBufferStatus:
	 * @ARV_BUFFER_STATUS_UNKNOWN: unknown status
	 * @ARV_BUFFER_STATUS_SUCCESS: the buffer contains a valid image
	 * @ARV_BUFFER_STATUS_CLEARED: the buffer is cleared
	 * @ARV_BUFFER_STATUS_TIMEOUT: timeout was reached before all packets are received
	 * @ARV_BUFFER_STATUS_MISSING_PACKETS: stream has missing packets
	 * @ARV_BUFFER_STATUS_WRONG_PACKET_ID: stream has packet with wrong id
	 * @ARV_BUFFER_STATUS_SIZE_MISMATCH: the received image didn't fit in the buffer data space
	 * @ARV_BUFFER_STATUS_FILLING: the image is currently being filled
	 * @ARV_BUFFER_STATUS_ABORTED: the filling was aborted before completion
	 */


	if(arv_buffer_get_status(buffer)==ARV_BUFFER_STATUS_UNKNOWN)
		std::cout << "AravisCamera: ARV_BUFFER_STATUS_UNKNOWN" << std::endl;
	if(arv_buffer_get_status(buffer)==ARV_BUFFER_STATUS_CLEARED)
		std::cout << "AravisCamera: ARV_BUFFER_STATUS_CLEARED" << std::endl;
	if(arv_buffer_get_status(buffer)==ARV_BUFFER_STATUS_TIMEOUT)
		std::cout << "AravisCamera: ARV_BUFFER_STATUS_TIMEOUT" << std::endl;
	if(arv_buffer_get_status(buffer)==ARV_BUFFER_STATUS_MISSING_PACKETS)
		std::cout << "AravisCamera: ARV_BUFFER_STATUS_MISSING_PACKETS" << std::endl;
	if(arv_buffer_get_status(buffer)==ARV_BUFFER_STATUS_WRONG_PACKET_ID)
		std::cout << "AravisCamera: ARV_BUFFER_STATUS_WRONG_PACKET_ID" << std::endl;
	if(arv_buffer_get_status(buffer)==ARV_BUFFER_STATUS_SIZE_MISMATCH)
		std::cout << "AravisCamera: ARV_BUFFER_STATUS_SIZE_MISMATCH" << std::endl;
	if(arv_buffer_get_status(buffer)==ARV_BUFFER_STATUS_FILLING)
		std::cout << "AravisCamera: ARV_BUFFER_STATUS_FILLING" << std::endl;
	if(arv_buffer_get_status(buffer)==ARV_BUFFER_STATUS_ABORTED)
		std::cout << "AravisCamera: ARV_BUFFER_STATUS_ABORTED" << std::endl;


	// make sure we have a valid frame
	if(buffer != NULL && arv_buffer_get_status(buffer) == ARV_BUFFER_STATUS_SUCCESS) {
		size_t thesize;
		const void * thedata = arv_buffer_get_data(buffer, &thesize);
		//image.get_allocator().allocate(thesize);
		//write image - make sure to use -fpermissive flag!!!
		//image.assign(thesize,thedata);
		//image = *(vector<unsigned char> *)thedata;
		//for(int i = 0; i < thesize; i++)
		//	image[i] = *(const char *)thedata[i];
		//if(buffer != NULL && buffer->status == ARV_BUFFER_STATUS_SUCCESS)
		image.assign((guint8 *)thedata, (guint8 *)thedata + thesize);
	}
    else{
		std::cout << "AravisCamera: Buffer cleared" << std::endl;
        image.clear();
        return image;
	}

	// push buffer back into stream and return
	arv_stream_push_buffer(stream, buffer);
	return image;
}

string AravisCamera::getID() {
	return cameraID;
}

int AravisCamera::getPayload() {
	payload = arv_camera_get_payload(camera);
	return payload;
}

int AravisCamera::getGain() {
	return arv_camera_get_gain(camera);
}

double AravisCamera::getExposure() {
	return arv_camera_get_exposure_time(camera);
}

double AravisCamera::getFrameRate() {
	return arv_camera_get_frame_rate(camera);
}

int AravisCamera::getPixelDepth() {
   	ArvPixelFormat format = arv_camera_get_pixel_format(camera);
	switch(format) {
	case ARV_PIXEL_FORMAT_MONO_8:
		pixelDepth = 8;
		break;
	case ARV_PIXEL_FORMAT_MONO_16:
		pixelDepth = 16;
		break;
	default:
		pixelDepth = -1;
	}
	return pixelDepth;
}

bool AravisCamera::setGain(int gain) {
	arv_camera_set_gain(camera, gain);
	if(getGain() != gain)
		return false;
	return true;
}

bool AravisCamera::setExposure(double exposure_us) {
	arv_camera_set_exposure_time(camera, exposure_us);
	if(getExposure() != exposure_us)
		return false;
	return true;
}

bool AravisCamera::setFrameRate(double rate_hz) {
	arv_camera_set_frame_rate(camera, rate_hz);
	if(getFrameRate() != rate_hz)
		return false;
	return true;
}

void AravisCamera::setGainAuto(bool useAuto, bool continuous) {
	if(useAuto && continuous)
		arv_camera_set_gain_auto(camera, ARV_AUTO_CONTINUOUS);
	else if(useAuto && !continuous)
		arv_camera_set_gain_auto(camera, ARV_AUTO_ONCE);
	else
		arv_camera_set_gain_auto(camera, ARV_AUTO_OFF);
}

void AravisCamera::setExposureAuto(bool useAuto, bool continuous) {
	if(useAuto && continuous)
		arv_camera_set_exposure_time_auto(camera, ARV_AUTO_CONTINUOUS);
	else if(useAuto && !continuous)
		arv_camera_set_exposure_time_auto(camera, ARV_AUTO_ONCE);
	else
		arv_camera_set_exposure_time_auto(camera, ARV_AUTO_OFF);
}

bool AravisCamera::setPixelDepth(int depth) {
	if(depth == 8)
		arv_camera_set_pixel_format(camera, ARV_PIXEL_FORMAT_MONO_8);
	else if(depth == 16)
		arv_camera_set_pixel_format(camera, ARV_PIXEL_FORMAT_MONO_16);
	if(getPixelDepth() != depth)
		return false;
	getPayload();
	return true;
}
