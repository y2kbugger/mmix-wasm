initialMixalProgram = `	LOC	80
	GREG	@
mask	OCTA	#0102040810204080
output	BYTE	"UUUUUUUU",10,0,0,0,0,0,0,0
#input	OCTA	#aabb112233445566
input	BYTE	"y2kbuggr"

	LOC	#100
	GREG	@
x	IS	$1
y	IS	$2
z	IS	$3
Main	LDOU	y,input
	LDOU	z,mask
	MXOR	x,y,z
	STOU	x,output
	LDA	$255,output
	TRAP	0,Fputs,StdOut
	TRAP	0,Halt,0`

function makeWriter(elementid) {
    var element = document.getElementById(elementid);
    if (element) element.value = ''; // clear browser cache
    return function(text) {
      if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
      if (text == "Calling stub instead of signal()") return
      if (element) {
        element.value += text + "\n";
        element.scrollTop = element.scrollHeight; // focus on bottom
      }
      // if we need to put these at console for some reason
    };
}

var inputElement = document.getElementById('input');
var mmix;
var mmixal;

var rerunOldVal = "";
function rerun(){
    if (inputElement.value == rerunOldVal) return;

    // store the program across refreshes
    MixalProgram = localStorage.setItem('MixalProgram',inputElement.value);

    document.getElementById('error').value = "";
    FS.writeFile("/y2k.mms", inputElement.value);
    mmixal();
    if (document.getElementById('error').value == "")
    {
        // mmix_sim();
        document.getElementById('output').value = "";
        mmix_sim_js()
    }
    rerunOldVal = inputElement.value;
}

sim_timer = undefined;
instruction_count = 0;
function mmix_sim_js(){
    if (sim_timer != undefined) {
        mmix_sim_js_stop()
    }

    instruction_count = 0;
    $('#instruction-count').text(instruction_count);

    mmix_lib_initialize();
    mmix_initialize();
    mmix_boot();
    y2k_mmix_load_file();

    interval_ms = 1000 / $('#freq').val()
    sim_timer = setInterval(mmix_sim_loop, interval_ms);
    // sim_timer = setInterval(mmix_sim_loop, 1000);

    // set_breakpoint(0);
    // if(get_interacting())
    // {
    //     mmix_interact();
    // }
}


function mmix_sim_js_stop(){
    clearInterval(sim_timer);
    sim_timer = undefined;
    mmix_finalize();
}

function mmix_sim_loop() {
    // if(get_interrupt()&&!get_breakpoint())
    // {
    //     set_breakpoint(get_breakpoint() | get_trace_bit());
    //     set_interacting(true);
    //     set_interrupt(false);
    // }
    // else
    // {
    //     set_breakpoint(0);
    //     if(get_interacting())
    //     {
    //         mmix_interact();
    //     }
    // }
    console.log("Outer_looper");
    if (get_halted())
    {
        console.log('halting');
        show_stats(true);
        mmix_sim_js_stop();
        return;
    }

    // do {
    // This inner loop is normal running
    // The out loop (this whole function is the part handling interrupt resumes and breakpoints
        console.log('Inner_looper');
        if(!get_resuming())
        {
            mmix_fetch_instruction();
            instruction_count += 1;
            $('#instruction-count').text(instruction_count);
        }
        mmix_perform_instruction();
        reg_num = $('#g_reg_num').val();
        $('#g_reg_val').html(g_hex(reg_num));
        // mmix_trace();
        mmix_dynamic_trap();
        // if(get_resuming() && op != RESUME)
        // fixme we need to add back this op checking
        if(get_resuming())
        {
            set_resuming(false);
        }
    // } while ((get_resuming() || (!get_interrupt() && !get_breakpoint())))

    // mmix_sim_js_stop();

    // if(get_interact_after_break())
    // {
    //     set_interacting(true);
    //     set_interact_after_break(false);
    // }

}

// Module = new Object();
Module.print = makeWriter('output');
Module.printErr = makeWriter('error');
Module.onRuntimeInitialized = function (){
    // wait until runtimes are loaded to map these
    mmix_sim = Module.cwrap('mmix_sim', 'number', []);
    mmixal = Module.cwrap('mmixal_wasm', 'number', []);
    halted = Module.cwrap('halted_wasm', 'number', []);
    print_mystr = Module.cwrap('print_mystr', 'number', ['string']);
    mmix_initialize_globals = Module.cwrap('mmix_initialize_globals', 'number', []);
    y2k_mmix_load_file = Module.cwrap('y2k_mmix_load_file', null, []);

    mmix_lib_initialize = Module.cwrap('mmix_lib_initialize', 'number', []);
    mmix_initialize = Module.cwrap('mmix_initialize', 'number', []);
    mmix_boot = Module.cwrap('mmix_boot', 'number', []);
    mmix_load_file = Module.cwrap('mmix_boot', 'number', ['string']);
    mmix_interact = Module.cwrap('mmix_interact', 'number', ['string']);

    mmix_fetch_instruction = Module.cwrap('mmix_fetch_instruction', 'number', []);
    mmix_perform_instruction = Module.cwrap('mmix_perform_instruction', 'number', []);
    mmix_trace = Module.cwrap('mmix_trace', 'number', []);
    mmix_dynamic_trap = Module.cwrap('mmix_dynamic_trap', 'number', []);
    show_stats = Module.cwrap('show_stats', 'number', []);
    mmix_finalize = Module.cwrap('mmix_finalize', 'number', []);


    get_halted = Module.cwrap('get_halted', 'bool', []);
    set_halted = Module.cwrap('set_halted', null, ['bool']);

    get_breakpoint = Module.cwrap('get_breakpoint', 'number', []);
    set_breakpoint = Module.cwrap('set_breakpoint', null, ['number']);

    get_interrupt = Module.cwrap('get_interrupt', 'bool', []);
    set_interrupt = Module.cwrap('set_interrupt', null, ['bool']);

    get_resuming = Module.cwrap('get_resuming', 'bool', []);
    set_resuming = Module.cwrap('set_resuming', null, ['bool']);

    get_interacting = Module.cwrap('get_interacting', 'bool', []);
    set_interacting = Module.cwrap('set_interacting', null, ['bool']);

    get_interact_after_break = Module.cwrap('get_interact_after_break', 'bool', []);
    set_interact_after_break = Module.cwrap('set_interact_after_break', null, ['bool']);

    get_profiling = Module.cwrap('get_profiling', 'bool', []);
    set_profiling = Module.cwrap('set_profiling', null, ['bool']);

    get_showing_stats = Module.cwrap('get_showing_stat', 'bool', []);
    set_showing_stats = Module.cwrap('set_showing_stat', null, ['bool']);

    get_trace_bit = Module.cwrap('get_trace_bit', 'number', []);
    get_general_register = Module.cwrap('get_general_register', 'number', ['number','bool']);

    MixalProgram = localStorage.getItem('MixalProgram');
    if (MixalProgram == null) {
        // Set up initial program and run once
        MixalProgram = initialMixalProgram;
    }
    inputElement.value = MixalProgram;
    rerun();
}
$("#input").on('keyup paste', rerun);
document.getElementById("input").addEventListener("keydown", function(e) {
    if (e.which===9) {
        e.preventDefault();
        document.execCommand("insertText", false, "\t");
    }
}, false);

function g_hex(register_num, noformat) {
    h = (get_general_register(register_num, true) >>>0).toString(16);
    l = (get_general_register(register_num, false) >>>0).toString(16);
    hs = '0'.repeat(8 - h.length);
    ls = '0'.repeat(8 - l.length);
    if (noformat) return hs + h + ls + l;
    if (h == '0') {
        return hs + h + ls + '<b>' + l + '</b>';
    }
    else {
        return hs + '<b>' + h + ls +  l +'</b>';
    }
}
function g_bin(register_num, noformat) {
    h = (get_general_register(register_num, true) >>>0).toString(2);
    l = (get_general_register(register_num, false) >>>0).toString(2);
    hs = '0'.repeat(32 - h.length);
    ls = '0'.repeat(32 - l.length);
    if (noformat) return hs + h + ls + l;
    if (h == '0') {
        return hs + h + ls + '<b>' + l + '</b>';
    }
    else {
        return hs + '<b>' + h + ls +  l +'</b>';
    }
}
function g_dec(register_num, noformat) {
    if (h != 0) return "overflow"; // this could be handled at some point
    // h = get_g_register_h(register_num).toString(2);
    l = get_g_register_l(register_num).toString(10);
    if (noformat) return l;

    return '<b>' + l + '</b>';
}


// https://stackoverflow.com/questions/6637341/use-tab-to-indent-in-textarea

// // Create a Web Worker (separate thread) that we'll pass the WebAssembly module to.
//  var g_WebWorker = new Worker("webworker.js");
//  g_WebWorker.onerror = function (evt) { console.log(`Error from Web Worker: ${evt.message}`); }
//  g_WebWorker.onmessage = function (evt) { alert(`Message from the Web Worker:\n\n ${evt.data}`); }
//  // Request the wasm file from the server and compile it...(Typically we would call
//  // 'WebAssembly.instantiate' which compiles and instantiates the module. In this
//  // case, however, we just want the compiled module which will be passed to the Web
//  // Worker. The Web Worker will be responsible for instantiating the module.)
//  fetch("mmix.wasm").then(response =>
//    response.arrayBuffer()
//  ).then(bytes =>
//    WebAssembly.compile(bytes)
//  ).then(WasmModule =>
//    g_WebWorker.postMessage({ "MessagePurpose": "CompiledModule", "WasmModule": WasmModule })
//  );
//  function OnClickTest() {
//    // Ask the Web Worker to add two values
//    g_WebWorker.postMessage({ "MessagePurpose": "AddValues", "Val1": 1, "Val2": 2 });
//  }
