#!/usr/bin/env python
#this file isn't ready to use now - gsm-receiver lacks realtime processing capability 
#there are many underruns of buffer for samples from usrp's, many blocks of samples get lost and
#receiver isn't prepared for this situation too well

from gnuradio import gr, gru, blks2, eng_notation
#, gsm
from gnuradio import usrp
from gnuradio.eng_option import eng_option
from optparse import OptionParser
from os import sys

for extdir in ['../../debug/src/lib','../../debug/src/lib/.libs','../lib','../lib/.libs']:
    if extdir not in sys.path:
        sys.path.append(extdir)
import gsm                        

class tuner(gr.feval_dd):
    def __init__(self, top_block):
        gr.feval_dd.__init__(self)
        self.top_block = top_block
    def eval(self, freq_offet):
        self.top_block.set_center_frequency(freq_offet)
        return freq_offet
        
class synchronizer(gr.feval_dd):
    def __init__(self, top_block):
        gr.feval_dd.__init__(self)
        self.top_block = top_block

    def eval(self, timing_offset):
        self.top_block.set_timing(timing_offset)
        return freq_offet

class gsm_receiver_first_blood(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)
        (options, args) = self._process_options()
        self.tuner_callback = tuner(self)
        self.synchronizer_callback = synchronizer(self)
        self.options    = options
        self.args       = args
        self._set_rates()
        self.source = self._set_source()
        self.filtr = self._set_filter()
        self.interpolator = self._set_interpolator()
        self.receiver = self._set_receiver()
        self.converter = self._set_converter()
        self.sink = self._set_sink()
    
        self.connect(self.source, self.filtr,  self.interpolator, self.receiver, self.converter, self.sink)
  
    def _set_sink(self):
        nazwa_pliku_wy = self.options.outputfile
        ujscie = gr.file_sink(gr.sizeof_float, nazwa_pliku_wy)
        return ujscie
    
    def _set_source(self):
        options = self.options
        fusb_block_size = gr.prefs().get_long('fusb', 'block_size', 4096)
        fusb_nblocks    = gr.prefs().get_long('fusb', 'nblocks', 16)
        self.usrp = usrp.source_c(decim_rate=options.decim, fusb_block_size=fusb_block_size, fusb_nblocks=fusb_nblocks)
        
        if options.rx_subdev_spec is None:
            options.rx_subdev_spec = usrp.pick_rx_subdevice(self.usrp)
        
        self.usrp.set_mux(usrp.determine_rx_mux_value(self.usrp, options.rx_subdev_spec))
        # determine the daughterboard subdevice
        self.subdev = usrp.selected_subdev(self.usrp, options.rx_subdev_spec)
	print "Using Rx d'board %s" % (self.subdev.side_and_name(),)
        input_rate = self.usrp.adc_freq() / self.usrp.decim_rate()
	print "USB sample rate %s" % (eng_notation.num_to_str(input_rate))

        # set initial values
        if options.gain is None:
            # if no gain was specified, use the mid-point in dB
            g = self.subdev.gain_range()
            options.gain = float(g[0]+g[1])/2

        r = self.usrp.tune(0, self.subdev, options.freq)
        self.subdev.set_gain(options.gain)
        return self.usrp
    
    def _set_rates(self):
        options = self.options
        clock_rate = 64e6
        self.clock_rate = clock_rate
        self.input_rate = clock_rate / options.decim
        self.gsm_symb_rate = 1625000.0 / 6.0
        self.sps = self.input_rate / self.gsm_symb_rate / self.options.osr

    def _set_filter(self):
        filter_cutoff   = 145e3	
        filter_t_width  = 10e3
        offset = 0
#        print "input_rate:", self.input_rate, "sample rate:", self.sps, " filter_cutoff:", filter_cutoff, " filter_t_width:", filter_t_width
        filter_taps     = gr.firdes.low_pass(1.0, self.input_rate, filter_cutoff, filter_t_width, gr.firdes.WIN_HAMMING)
        filtr          = gr.freq_xlating_fir_filter_ccf(1, filter_taps, offset, self.input_rate)
        return filtr

    def _set_converter(self):
        v2s = gr.vector_to_stream(gr.sizeof_float, 142)
        return v2s
    
    def _set_interpolator(self):
        interpolator = gr.fractional_interpolator_cc(0, self.sps) 
        return interpolator
    
    def _set_receiver(self):
        receiver = gsm.receiver_cf(self.tuner_callback, self.synchronizer_callback, self.options.osr, self.options.key.replace(' ', '').lower(), self.options.configuration.upper())
        return receiver
    
    def _process_options(self):
        parser = OptionParser(option_class=eng_option)
        parser.add_option("-d", "--decim", type="int", default=112,
                                    help="Set USRP decimation rate to DECIM [default=%default]")
        parser.add_option("-r", "--osr", type="int", default=4,
                          help="Oversampling ratio [default=%default]")
        parser.add_option("-I", "--inputfile", type="string", default="cfile",
                                    help="Input filename")
        parser.add_option("-O", "--outputfile", type="string", default="cfile2.out",
                                    help="Output filename")
        parser.add_option("-R", "--rx-subdev-spec", type="subdev", default=None,
                                    help="Select USRP Rx side A or B (default=first one with a daughterboard)")
        parser.add_option("-f", "--freq", type="eng_float", default="950.4M",
                                    help="set frequency to FREQ", metavar="FREQ")
        parser.add_option("-g", "--gain", type="eng_float", default=None,
                                    help="Set gain in dB (default is midpoint)")
        parser.add_option("-k", "--key", type="string", default="AD 6A 3E C2 B4 42 E4 00",
                          help="KC session key")
        parser.add_option("-c", "--configuration", type="string", default="",
                          help="Decoder configuration")

        (options, args) = parser.parse_args ()
        return (options, args)
    
    def set_center_frequency(self, center_freq):
        self.filtr.set_center_freq(center_freq)

    def set_timing(self, timing_offset):
        pass

def main():
    try:
        gsm_receiver_first_blood().run()
    except KeyboardInterrupt:
        pass

if __name__ == '__main__':
    main()
