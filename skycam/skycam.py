import sys
sys.path.append("/usr/alignment/opencv-3.4.3/lib/python2.7/site-packages/")

try:
    import gi
    from gi.repository import Aravis
    import cv2
except:
    print("Please make sure you ran \"set_skycam\"")
    print("which executes \"export GI_TYPELIB_PATH=$GI_TYPELIB_PATH:/home/ctauser/skycam/aravis/src\"")
    print("and \"export LD_LIBRARY_PATH=/home/ctauser/skycam/aravis/src/.libs\"")

import ctypes
import numpy as np

from datetime import datetime
from pytz import timezone
import time
import logging
import argparse
import subprocess
import os
import signal
#import shutil
import re
#from threading import Timer

from opcua import ua, Server
#the client is used to get RA Dec from the drive system
from opcua import Client


#Let's try ignore DeprecationWarnings
import warnings
warnings.filterwarnings("ignore", category=DeprecationWarning)

#input timeout in case the script is run during the day
import select


#import importlib
#astropy_spec = importlib.util.find_spec("astropy")
#found_astropy = astropy_spec is not None

#if found_astropy:
try:
    import astropy.units as u
    from astropy.time import Time
    from astropy.coordinates import SkyCoord, EarthLocation, AltAz


    def RaDec2AzEl(center_ra=116.150876, center_dec=72.198172, ut_time='2018-02-27 02:11:19',
                   observer_lat=31.674997676, observer_long=-110.9521311, observer_alt=1268,
                   temperature=20, relative_humidity=20, pressure=1020, verbose=True):
        observer_location = EarthLocation.from_geodetic(lat=observer_lat * u.deg, lon=observer_long * u.deg,
                                                        height=observer_alt * u.m)

        FoV_center_coord = SkyCoord(ra=center_ra * u.deg, dec=center_dec * u.deg)
        observe_time = Time(ut_time)

        altaz = FoV_center_coord.transform_to(AltAz(obstime=observe_time, location=observer_location,
                                                    pressure=pressure * u.hPa, relative_humidity=relative_humidity,
                                                    temperature=temperature * u.Celsius))
        if verbose:
            print("Elevataion = {0.alt:.5}".format(altaz))
            print("Azimuth = {0.az:.5}".format(altaz))
        return altaz.alt.deg, altaz.az.deg
        #return float("{0.alt:.5}".format(altaz)), float("{0.az:.5}".format(altaz))

except:
    print("Can't import astropy")
# Importing astrometry.net wrapper:
#from py_astrom import *



def setup_logger(logger_name, log_file, level=logging.INFO, show_log=True):
    log_setup = logging.getLogger(logger_name)
    formatter = logging.Formatter('%(levelname)s: %(asctime)s %(message)s', datefmt='%Y-%m-%d %I:%M:%S %p')
    fileHandler = logging.FileHandler(log_file, mode='a')
    fileHandler.setFormatter(formatter)
    streamHandler = logging.StreamHandler()
    streamHandler.setFormatter(formatter)
    log_setup.setLevel(level)
    log_setup.addHandler(fileHandler)
    log_setup.addHandler(streamHandler)
    if show_log:
        ch = logging.StreamHandler(sys.stdout)
        ch.setLevel(logging.DEBUG)
        formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
        ch.setFormatter(formatter)
        log_setup.addHandler(ch)


def get_opcua_coord_children():
    plc = "10.0.50.111"      # pSCT
    client = Client("opc.tcp://" + plc + ":4840/Objects/Logic/")
    root = client.get_root_node()
    RA_child = root.get_child(["0:Objects", "2:Logic", "2:Application", "2:UserVarGlobal_OPCUA", "2:current_RA"])
    Dec_child = root.get_child(["0:Objects", "2:Logic", "2:Application", "2:UserVarGlobal_OPCUA", "2:current_Dec"])
    time_child = root.get_child(["0:Objects", "2:Logic", "2:Application", "2:UserVarGlobal_OPCUA", "2:current_time"])
    return time_child, RA_child, Dec_child


def raw_input_with_timeout(prompt, timeout=5.0):
    print prompt,    
    timer = threading.Timer(timeout, thread.interrupt_main)
    astring = None
    try:
        timer.start()
        astring = raw_input(prompt)
    except KeyboardInterrupt:
        pass
    timer.cancel()
    return astring


def start_skycam(skycam_name='The Imaging Source Europe GmbH-DMK 23GP031-37514084'):
    if skycam_name not in get_device_ids():
        raise AravisException("Error: the skycam %s was not found", skycam_name)
    skycam = Camera(skycam_name)


def check_skycam(skycam_name='The Imaging Source Europe GmbH-DMK 23GP031-37514084'):
    found=True
    if skycam_name not in get_device_ids():
        found=False
        #raise AravisException("Error: the skycam %s was not found", skycam_name)
    return found


def get_device_ids():
    Aravis.update_device_list()
    n = Aravis.get_n_devices()
    return [Aravis.get_device_id(i) for i in range(0, n)]


def show_frame(frame):
    cv2.imshow("capture", frame)
    cv2.waitKey(0)


def save_frame(frame, path="frame.jpeg"):
    print("Saving frame to ", path)
    np.save(path, frame)


def sfn(cam, path="frame.jpeg"):
    from PIL import Image
    cam.start_acquisition()
    frame = cam.pop_frame()
    cam.stop_acquisition()
    im = Image.fromarray(frame)
    print("Saving image to ", path)
    im.save(path)


def get_frame(cam):
    cam.start_acquisition()
    frame = cam.pop_frame()
    cam.stop_acquisition()
    return frame


def convert(buf):
    if not buf:
        return None
    pixel_format = buf.get_image_pixel_format()
    bits_per_pixel = pixel_format >> 16 & 0xff
    if bits_per_pixel == 8:
        INTP = ctypes.POINTER(ctypes.c_uint8)
    else:
        INTP = ctypes.POINTER(ctypes.c_uint16)
    addr = buf.get_data()
    ptr = ctypes.cast(addr, INTP)
    im = np.ctypeslib.as_array(ptr, (buf.get_image_height(), buf.get_image_width()))
    im = im.copy()
    return im



class AravisException(Exception):
    pass


class Camera(object):
    """
    Create a Camera object. 
    name is the camera ID in aravis.
    If name is None, the first found camera is used.
    If no camera is found an AravisException is raised.
    """

    def __init__(self, name=None, 
                 exposure=500000, gain=15, framerate=10, 
                 loglevel=logging.WARNING):
        self.logger = logging.getLogger(self.__class__.__name__)
        if len(logging.root.handlers) == 0:  # dirty hack
            logging.basicConfig()
        self.logger.setLevel(loglevel)
        self.name = name
        self.skycam_name='The Imaging Source Europe GmbH-DMK 23GP031-37514084'
        if name is None:
            self.name = self.skycam_name
        try:
            self.cam = Aravis.Camera.new(name)
        except TypeError:
            if name:
                raise AravisException("Error the camera %s was not found or it's being used by another process", name)
            else:
                raise AravisException("Error no camera found")
        #self.name = self.cam.get_model_name()
        self.logger.info("Camera object created for device: %s", self.name)
        self.dev = self.cam.get_device()
        self.stream = self.cam.create_stream(None, None)
        if self.stream is None:
            raise AravisException("Error creating buffer")
        self._frame = None
        self._last_payload = 0
        self.get_exposure()
        self.get_framerate()
        self.get_gain()
        if self.exposure != exposure:
            self.set_exposure(exposure)
            #print("Setting exposure to {}".format(exposure))
        if self.gain != gain:
            self.set_gain(gain)
        if self.framerate != framerate:
            self.set_framerate(framerate)


    def __getattr__(self, name):
        if hasattr(self.cam, name):  # expose methods from the aravis camera object which is also relatively high level
            return getattr(self.cam, name)
        # elif hasattr(self.dev, name): #epose methods from the aravis device object, this might be confusing
        #    return getattr(self.dev, name)
        else:
            raise AttributeError(name)

    def __dir__(self):
        tmp = list(self.__dict__.keys()) + self.cam.__dir__()  # + self.dev.__dir__()
        return tmp

    def get_exposure(self):
        self.exposure = self.cam.get_exposure_time()
        return self.exposure

    def set_exposure(self, exposure):
        self.cam.set_exposure_time(exposure)
        self.exposure = self.get_exposure_time()
        print("Setting exposure time to {} us".format(self.exposure))

    def get_framerate(self):
        self.framerate = self.cam.get_frame_rate()
        return self.framerate

    def set_framerate(self, framerate):
        self.cam.set_frame_rate(framerate)
        self.framerate = self.get_framerate()
        print("Setting frame rate to {} Hz".format(self.framerate))

    def get_gain(self):
        self.gain=self.cam.get_gain()
        return self.gain

    def set_gain(self, gain):
        self.cam.set_gain(gain)
        self.gain = self.get_gain()
        print("Setting gain to {}".format(self.gain))

    def get_feature_type(self, name):
        genicam = self.dev.get_genicam()
        node = genicam.get_node(name)
        if not node:
            raise AravisException("Feature {} does not seem to exist in camera".format(name))
        return node.get_node_name()

    def get_feature(self, name):
        """
        return value of a feature. independantly of its type
        """
        ntype = self.get_feature_type(name)
        if ntype in ("Enumeration", "String", "StringReg"):
            return self.dev.get_string_feature_value(name)
        elif ntype == "Integer":
            return self.dev.get_integer_feature_value(name)
        elif ntype == "Float":
            return self.dev.get_float_feature_value(name)
        elif ntype == "Boolean":
            return self.dev.get_integer_feature_value(name)
        else:
            self.logger.warning("Feature type not implemented: %s", ntype)

    def set_feature(self, name, val):
        """
        set value of a feature
        """
        ntype = self.get_feature_type(name)
        if ntype in ("String", "Enumeration", "StringReg"):
            return self.dev.set_string_feature_value(name, val)
        elif ntype == "Integer":
            return self.dev.set_integer_feature_value(name, int(val))
        elif ntype == "Float":
            return self.dev.set_float_feature_value(name, float(val))
        elif ntype == "Boolean":
            return self.dev.set_integer_feature_value(name, int(val))
        else:
            self.logger.warning("Feature type not implemented: %s", ntype)

    def get_genicam(self):
        """
        return genicam xml from the camera
        """
        return self.dev.get_genicam_xml()

    def get_feature_vals(self, name):
        """
        if feature is an enumeration then return possible values
        """
        ntype = self.get_feature_type(name)
        if ntype == "Enumeration":
            return self.dev.get_available_enumeration_feature_values_as_strings(name)
        else:
            raise AravisException("{} is not an enumeration but a {}".format(name, ntype))

    def read_register(self, address):
        return self.dev.read_register(address)

    def write_register(self, address, val):
        return self.dev.write_register(address, val)

    def create_buffers(self, nb=10, payload=None):
        if not payload:
            payload = self.cam.get_payload()
        self.logger.info("Creating %s memory buffers of size %s", nb, payload)
        for _ in range(0, nb):
            self.stream.push_buffer(Aravis.Buffer.new_allocate(payload))

    def pop_frame(self, timestamp=False):
        while True:  # loop in python in order to allow interrupt, have the loop in C might hang
            if timestamp:
                ts, frame = self.try_pop_frame(timestamp)
            else:
                frame = self.try_pop_frame()

            if frame is None:
                time.sleep(0.001)
            else:
                if timestamp:
                    return ts, frame
                else:
                    return frame

    def try_pop_frame(self, timestamp=False):
        """
        return the oldest frame in the aravis buffer
        """
        buf = self.stream.try_pop_buffer()
        if buf:
            frame = self._array_from_buffer_address(buf)
            self.stream.push_buffer(buf)
            if timestamp:
                #return buf.get_timestamp(), frame
                timestamp_az = timezone('America/Phoenix').localize(datetime.fromtimestamp(buf.get_system_timestamp()*1e-9))
                timestamp_utc = timestamp_az.astimezone(timezone('UTC'))
                #return datetime.fromtimestamp(buf.get_system_timestamp()*1e-9).strftime("%Y-%m-%d_%H:%M:%S.%f"), frame
                return timestamp_utc.strftime("%Y-%m-%d_%H:%M:%S.%f"), frame
            else:
                return frame
        else:
            if timestamp:
                return None, None
            else:
                return None

    def try_save_frame(self, timestamp=True, work_dir='./'):
        #timestamp, frame = self.try_pop_frame(timestamp=timestamp)
        timestamp, frame = self.pop_frame(timestamp=timestamp)
        #print(timestamp, frame)
        outfile = os.path.join(work_dir, "skycam_image{}.jpeg".format(timestamp))
        if buffer:
            cv2.imwrite(outfile, frame)
        else:
            self.logger.error("*** Failed to save image skycam_image{}.jpeg ***".format(timestamp))
        return outfile, timestamp

    def _array_from_buffer_address(self, buf):
        if not buf:
            return None
        pixel_format = buf.get_image_pixel_format()
        bits_per_pixel = pixel_format >> 16 & 0xff
        if bits_per_pixel == 8:
            INTP = ctypes.POINTER(ctypes.c_uint8)
        else:
            INTP = ctypes.POINTER(ctypes.c_uint16)
        addr = buf.get_data()
        ptr = ctypes.cast(addr, INTP)
        im = np.ctypeslib.as_array(ptr, (buf.get_image_height(), buf.get_image_width()))
        im = im.copy()
        return im

    def trigger(self):
        """
        trigger camera to take a picture when camera is in software trigger mode
        """
        self.execute_command("TriggerSoftware")

    def __str__(self):
        return "Camera: " + self.name

    def __repr__(self):
        return self.__str__()

    def start_acquisition(self, nb_buffers=10):
        self.logger.info("starting acquisition")
        payload = self.cam.get_payload()
        if payload != self._last_payload:
            # FIXME should clear buffers
            self.create_buffers(nb_buffers, payload)
            self._last_payload = payload
        self.cam.start_acquisition()

    def start_acquisition_trigger(self, nb_buffers=1):
        self.set_feature("AcquisitionMode", "Continuous")  # no acquisition limits
        self.set_feature("TriggerSource", "Software")  # wait for trigger t acquire image
        self.set_feature("TriggerMode", "On")  # Not documented but necesary
        self.start_acquisition(nb_buffers)

    def start_acquisition_continuous(self, nb_buffers=20):
        self.set_feature("AcquisitionMode", "Continuous")  # no acquisition limits
        self.start_acquisition(nb_buffers)

    def stop_acquisition(self):
        self.cam.stop_acquisition()



def read_raw(f='./GAS_image.raw', cols=2592, rows=1944, outfile=None, show=False):
    fd = open(f, 'rb')
    f = np.fromfile(fd, dtype=np.uint8,count=rows*cols)
    im = f.reshape((rows, cols)) #notice row, column format
    fd.close()
    if outfile is not None:
        cv2.imwrite(outfile, im)
    if show:
        plt.imshow(im, cmap='gray')
    return im



class AstrometryScript:
    """calibrate a fits image with astrometry.net."""

    def __init__(self, raw_file=None, log_file=None, work_dir="./", scale_relative_error=0.05,
                 astrometry_bin='/usr/local/astrometry/bin/', cols=2592, rows=1944, jpeg_file=None,  
                 verbose=False
                ):
        self.log_file = log_file
        if log_file is not None:
            setup_logger("astrom_log", os.path.join(work_dir, log_file),
                         level=logging.INFO, show_log=verbose)
        else:
            setup_logger("astrom_log", os.path.join(work_dir, 'astrom_temp.log', show_log=verbose),
                         level=logging.INFO)
        self.logger = logging.getLogger('astrom_log')

        self.scale_relative_error = scale_relative_error
        self.astrometry_bin = astrometry_bin

        self.raw_file = raw_file
        self.work_dir = work_dir

        self.cols = cols
        self.rows = rows
        #self.infpath = os.path.join(self.work_dir, raw_file)
        #shutil.copy(self.raw_file, self.infpath)

        self.jpeg_file = jpeg_file
        self.jpeg_read = False

        self.__timeout__ = False


    @classmethod
    def getName(cls):
        return _("Local astrometry.net script")


    #def timeout(self):
    #    return self.__timeout__


    #def raise_timeout(self, signum, frame):
    #    raise TimeoutError


    #def timeout(self, time):
    #    # Register a function to raise a TimeoutError on the signal.
    #    signal.signal(signal.SIGALRM, self.raise_timeout)
    #    # Schedule the signal to be sent after ``time``.
    #    signal.alarm(self.time)
    #
    #    try:
    #        yield
    #    except TimeoutError:
    #        pass
    #    finally:
    #        # Unregister the signal so it won't be triggered
    #        # if the timeout is not reached.
    #        signal.signal(signal.SIGALRM, signal.SIG_IGN)



    def read_raw(self, jpeg_file=None, show=False):
        if jpeg_file is not None:
            self.jpeg_file = jpeg_file
        if self.jpeg_file is not None:
            self.logger.info("Saving image to file: {}".format(self.jpeg_file))
        im_ = read_raw(self.raw_file, cols=self.cols, rows=self.rows, outfile=self.jpeg_file, show=show)
        self.jpeg_read = True


    def run(self, scale=None, ra=None, dec=None, downsample=2, radius=5.0, replace=False, timeout=None, verbose=False, extension=None, plot=False, 
            center=False, wrkr=None):
        # '--no-verify: if not specified the output is different

        if not self.jpeg_read:
            self.logger.info("Reading raw file {}".format(self.raw_file))
            #self.read_raw(jpeg_file=os.path.join(self.work_dir, 'tmp_input.jpeg'))
            self.read_raw(jpeg_file=os.path.join(self.work_dir, self.jpeg_file))

        # older versions:
        # solve_field=[self.astrometry_bin + '/solve-field', '-D', self.work_dir,'--no-plots', '--no-fits2fits','--no-verify',]
        # solve_field = [self.astrometry_bin + '/solve-field', '-D', self.work_dir, '--no-plots', '--no-verify', ]

        self.downsample = downsample

        solve_field = [self.astrometry_bin + '/solve-field', '--overwrite', '-D', self.work_dir, '-N', 'none',
                       # '--no-plots','--no-verify',
                      ]

        if scale is not None:
            scale_low = scale * (1 - self.scale_relative_error)
            scale_high = scale * (1 + self.scale_relative_error)
            solve_field.append('-u')
            solve_field.append('app')
            solve_field.append('-L')
            solve_field.append(str(scale_low))
            solve_field.append('-H')
            solve_field.append(str(scale_high))

        if ra is not None and dec is not None:
            solve_field.append('--ra')
            solve_field.append(str(ra))
            solve_field.append('--dec')
            solve_field.append(str(dec))
            solve_field.append('--radius')
            solve_field.append(str(radius))

        if extension is not None:
            solve_field.append('-6')
            solve_field.append(str(extension))

        if center:
            solve_field.append('--crpix-center')

        if downsample is not None:
            solve_field.append('-z')
            solve_field.append(str(downsample))

        solve_field.append(self.jpeg_file)

        if verbose:
            self.logger.info('running: {}'.format(' '.join(solve_field)))

        if not plot:
            solve_field.append('--no-plots')

        if timeout is None:
            timeout=600 #Let's not let the script run for too long
        
        #astrometry_timer = Timer(timeout, kill, [ping])

        #with self.timeout(timeout):
            #def __term_proc(sig, stack):
            #    os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            #    if verbose:
            #        self.logger.warning('killing process, as timeout was reached')
            #    self.logger.error('{}: time out: {} sec reached, closing down'.format(wrkr, timeout))

            #self.__timeout__ = True
            #signal.signal(signal.SIGALRM, __term_proc)
            #signal.alarm(timeout)  # timeout in seconds


        proc = subprocess.Popen(solve_field, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                preexec_fn=lambda: os.setpgid(0, 0))
        #proc = subprocess.Popen(solve_field, stdout=None, stderr=None,
        #                        preexec_fn=lambda: os.setpgid(0, 0))



        radecline = re.compile('Field center: \(RA,Dec\) = \(([^,]*),(.*)\).*?')
        didnotsolve =  re.compile('Did not solve')
        yourerunningindaytime = re.compile('Failed to read xylist field.')

        err = proc.stderr.readline().decode('utf-8')

        if err is not None:
            # may be more
            if 'whaddup?' in err:
                pass
            elif 'jpegtopnm' in err: 
                pass
            else:
                self.logger.error('error from astrometry.net: {}'.format(err))
                #pass
                #raise
                #return None

        ret = None

        #line_limit=100000
        line_limit=2000
        line_c = 0
        while True:
            line_c+=1
            if line_c>line_limit:
                self.__timeout__ = True
                if verbose:
                    self.logger.warning('killing process, a timeout was reached while reading stdout')
                break
            try:
                a = proc.stdout.readline().decode('utf-8')
                #print(a)
            except IOError:
                break
            if a == '':
                break
            if verbose:
                #self.logger.debug(a)
                self.logger.info(a)

            match = radecline.match(a)
            if match:
                ret = [float(match.group(1)), float(match.group(2))]
                #print(ret)
                break
            didnotsolve_match = didnotsolve.match(a)
            if didnotsolve_match:
                print("Did not solve dot dot dot, dot dot")
                break
            readxyfailed=yourerunningindaytime.match(a)
            if readxyfailed:
                self.logger.error("Are you running during the day?")
                yes = {'yes','y', 'ye', 'Yes', 'Y'}
                no = {'no','n', 'N', 'No'}
                print("Are you running during the day? (y/n, you have 5 seconds)")
                #isday = raw_input().lower()
                isday, o, e = select.select( [sys.stdin], [], [], 5 )
                if isday in yes:
                    print("I'd better shut myself, wake me up after sunset...")
                    self.logger.error("I'd better shut myself, wake me up after sunset... bye!")
                    sys.exit()
                elif isday in no:
                    print("I hope you are not lying. Something seems wrong, better ctrl-c once, not twice, but once. I'll give you 3 seconds.")
                    self.logger.error("S/He said it's night time. Something seems wrong, better ctrl-c once, not twice, but once. I'll give you 3 seconds...")
                    time.sleep(3)
                    print("3 seconds is up. I'll just try again. ")
                    self.logger.error("3 seconds passed, will try again.")
                else:
                    #sys.stdout.write("Please respond with 'yes' or 'no'")
                    print("5 seconds is up, no input. I'm probably talking to nobody. I'll just try again. ")
                    self.logger.error("5 seconds passed, no input, will try again.")
                break

        #these two lines prevents printing to stdout
        proc.stdout.close()
        proc.stderr.close()

        if ret is None:
            self.logger.error("Did not solve field...")
        else:
            self.logger.info('Field center: (RA,Dec) =  ({}, {})'.format(ret[0], ret[1]))

        proc.kill()
        return ret



def run_skycam(args):
    setup_logger("skycam_log", os.path.join(args.work_dir, args.log_file),
                 level=logging.INFO, show_log=args.verbose)
    skycam_logger = logging.getLogger('skycam_log')

    skycam=Camera(name='The Imaging Source Europe GmbH-DMK 23GP031-37514084', loglevel=logging.WARNING, 
                  exposure=args.exposure, gain=args.gain, framerate=args.framerate)

    timestamp_az = timezone('America/Phoenix').localize(datetime.now())
    timestamp_utc = timestamp_az.astimezone(timezone('UTC'))
    workdir=os.path.join(args.data_outdir, timestamp_utc.strftime('%Y%m%d')) 

    if not os.path.exists(workdir):
        os.makedirs(workdir)
    if args.data_outfile is not None: 
        outfile=os.path.join(workdir, args.data_outfile)
        if os.path.exists(outfile):
            append_write = 'a' # append if already exists
        else:
            append_write = 'w' # make a new file if not
        outf = open(outfile, append_write)
        print("Saving results to output file {}".format(outfile))
        if append_write == 'w':
            if args.azel:
                outf.write("Timestamp,RA,Dec,Az,El\n")
            elif args.get_tracking:
                outf.write("Timestamp,RA,Dec,trackingRA,trackingDec\n")
            else: 
                outf.write("Timestamp,RA,Dec\n")
    try:
        #start opcua server
        #url="opc.tcp://10.0.50.113:4840/skycam/"
        url="opc.tcp://10.0.50.113:48800"
        skycam_logger.info("Starting opcua server for skycam at {}".format(url))
        #skycam_server = setup_opcua_server()
        skycam_server = Server()
        skycam_server.set_endpoint(url)
        #uri = "http://10.0.50.113:4840/skycam/"
        uri = "http://10.0.50.113:48800"
        idx = skycam_server.register_namespace(uri)
        objects = skycam_server.get_objects_node()
        skycam_obj = objects.add_object(idx, "skycam_pointing_data")
        skycam_timestamp = skycam_obj.add_variable(idx, "Timestamp", 0)
        skycam_ra = skycam_obj.add_variable(idx, "RA", 0.0)
        skycam_dec = skycam_obj.add_variable(idx, "Dec", 0.0)
        if args.get_tracking:
            tracking_ra = skycam_obj.add_variable(idx, "trackingRA", 0.0)
            tracking_dec = skycam_obj.add_variable(idx, "trackingDec", 0.0)
            #objs for reading RA and Dec from the drive system: 
            time_child, RA_child, Dec_child = get_opcua_coord_children()

        skycam_server.start()

        #print("Timestamp, RA, Dec")
        skycam_logger.info("Starting acquisition and solve-field processes forever...")


        while True:
            skycam.start_acquisition(1)
            #for i in range(20):
            #    figfile, timestamp = skycam.try_save_frame()
            #    #time.sleep(0.1)
            #    if timestamp is None: 
            #        print("Trouble saving frame, trying again...")
            #        skycam_logger.info("Trouble saving frame, trying again...")
            #    else:
            #        #print(timestamp)
            #        print("Framed saved to {}".format(figfile))
            #        skycam_logger.info("Framed saved to {}".format(figfile))
            #        break
            figfile, timestamp = skycam.try_save_frame(work_dir=workdir)
            figfile = os.path.join(workdir, figfile)
            print(figfile)
            #time.sleep(0.1)
            if timestamp is None:
                print("Trouble saving frame, trying again...")
                skycam_logger.info("Trouble saving frame, trying again...")
                skycam.stop_acquisition()
                skycam.pop_frame()
                continue
            else:
                #print(timestamp)
                print("Framed saved to {}".format(figfile))
                skycam_logger.info("Framed saved to {}".format(figfile))

            
            astrom = AstrometryScript(raw_file=figfile, log_file=args.log_file, work_dir=workdir,
                                      scale_relative_error=args.scale_relative_error, jpeg_file=figfile,
                                      astrometry_bin=args.astrometry_bin, cols=args.cols, rows=args.rows, 
                                      verbose=args.verbose)
            astrom.jpeg_read=True
            radec = astrom.run(scale=args.scale, ra=args.ra, dec=args.dec, downsample=args.downsample,
                       radius=args.radius, timeout=args.timeout, verbose=args.verbose,
                       extension=None, center=False, wrkr=None, plot=args.plot)
            if radec is None:
                print("This frame at time {} isn't solved".format(timestamp))
                skycam_logger.info("This frame at time {} isn't solved".format(timestamp))
            else:
                if args.azel:
                    az, el = RaDec2AzEl(center_ra=radec[0], center_dec=radec[1], ut_time=' '.join(timestamp.split('_')),
                                        observer_lat=31.674997676, observer_long=-110.9521311, observer_alt=1268,
                                        temperature=20, relative_humidity=20, pressure=1020, verbose=True)
                    print("{}, {}, {}, {}, {}".format(timestamp, radec[0], radec[1], az, el))
                    if args.data_outfile is not None: 
                        outf.write("{},{},{},{},{}\n".format(timestamp, radec[0], radec[1], az, el))
                elif args.get_tracking:
                    tracking_timestamp = time_child.get_value()
                    tracking_ra = RA_child.get_value()
                    tracking_dec = Dec_child.get_value()
                    print("{}, {}, {}, {}, {}".format(timestamp, radec[0], radec[1], tracking_ra, tracking_dec))
                    if args.data_outfile is not None: 
                        outf.write("{},{},{},{},{}\n".format(timestamp, radec[0], radec[1], tracking_ra, tracking_dec))
                else:
                    print("{}, {}, {}".format(timestamp, radec[0], radec[1]))
                    if args.data_outfile is not None: 
                        outf.write("{},{},{}\n".format(timestamp, radec[0], radec[1]))

                skycam_logger.info("{}, {}, {}".format(timestamp, radec[0], radec[1]))
                skycam_timestamp.set_value(timestamp)
                skycam_ra.set_value(radec[0])
                skycam_dec.set_value(radec[1])
            skycam.stop_acquisition()

            #time.sleep(0.1)
    except KeyboardInterrupt:
        print("Measurement stopped")
        pass
    finally:
        print("Stopping skycam acquisition and opcua server")
        skycam.stop_acquisition()
        skycam_server.stop()
        if args.data_outfile is not None: 
            outf.close()




if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='Measure astrometry for an images')

    #parser.add_argument('-i', '--input', default=None, help='Input raw image file; default is None so that camera stream is used. ')
    parser.add_argument('-w', '--work_dir', default="./", help='work dir')
    parser.add_argument('--scale_relative_error', type=float, default=0.05, help='scale_relative_error')
    parser.add_argument('--astrometry_bin', default='/usr/alignment/astrometry_install/bin/')
    parser.add_argument('--cols', type=int, default=2592, help='number of colums of the input image')
    parser.add_argument('--rows', type=int, default=1944, help='number of rows of the input image')
    parser.add_argument('-l', '--log_file', default="skycam_log.txt", help='log file name')
    parser.add_argument('-z', '--downsample', default=2, help='downsample rate')
    parser.add_argument('-r', '--radius', type=float, default=5.0, help='radius of FoV')
    parser.add_argument('--ra', default=None, help='RA of the FoV center')
    parser.add_argument('--dec', default=None, help='Dec of the FoV center')
    parser.add_argument('-s', '--scale', type=float, default=6.0582, help='plate scale')
    parser.add_argument('-o', '--output_file', default="test_skycam.jpeg", help='Name of the output jpeg file')
    parser.add_argument('-v', '--verbose', action='store_true', help='verbose flag')
    parser.add_argument('-p', '--plot', action='store_true', help='flag for saving plots, default is False')
    parser.add_argument('-t', '--timeout', default=None, help='Timeout settingns')
    parser.add_argument('-e', '--exposure', default=500000, help="Exposure time of the skycam, default is 500 ms, or 500000 us", type=float)
    parser.add_argument('-g', '--gain', default=15, help="Gain of the skycam, default is 15", type=float)
    parser.add_argument('-f', '--framerate', default=10, help="Frame rate of the skycam, default is 10", type=float)
    parser.add_argument('-a', '--azel', action='store_true', help='flag for printing Az, El to the screen. Default is False.')
    parser.add_argument('--get_tracking', action='store_true', help='flag for getting real time RA, Dec from the drive system. Default is False.')
    parser.add_argument('--data_outdir', default='/home/ctauser/skycam/data/')
    parser.add_argument('--data_outfile', default=None, help='Create a local file with all the clean output. Default is None. ')
    #parser.add_argument('-', '--', type=, default=, help='')


    args = parser.parse_args()

    run_skycam(args)

