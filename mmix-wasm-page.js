initialMixalProgram = `	LOC	#20
	GREG	@
argv	BYTE	"Hello,",10
	LOC	#100
	GREG	@
Main	LDA	$255,argv
	TRAP	0,Fputs,StdOut
	GETA	$255,String
	TRAP	0,Fputs,StdOut
	TRAP	0,Halt,0
String	BYTE	"	World",#a,0`;

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

    document.getElementById('output').value = "";
    document.getElementById('error').value = "";
    FS.writeFile("/y2k.mms", inputElement.value);
    mmixal();
    if (document.getElementById('error').value == "")
    {
        // mmix_sim();
        mmix_sim_js()
    }
    rerunOldVal = inputElement.value;
}

function mmix_sim_js(){
    mmix_lib_initialize();
    mmix_initialize();
    mmix_boot();
    y2k_mmix_load_file();

    // set_breakpoint(0);
    // if(get_interacting())
    // {
    //     mmix_interact();
    // }

    while(true)
    {
        console.log('start execution cycle');

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
        if (get_halted())
        {
            console.log('halting');
            break;
        }

        do
        {
            console.log('Inner looper');
            if(!get_resuming())
            {
                mmix_fetch_instruction();
            }
            mmix_perform_instruction();
            // mmix_trace();
            mmix_dynamic_trap();
            // if(get_resuming() && op != RESUME)
            if(get_resuming())
            {
                set_resuming(false);
            }


        } while(get_resuming()||(!get_interrupt()&&!get_breakpoint()));

        // if(get_interact_after_break())
        // {
        //     set_interacting(true);
        //     set_interact_after_break(false);
        // }
    }
    show_stats(true);
    mmix_finalize();
}

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

    inputElement.value = initialMixalProgram;
    rerun();
}
$("#input").on('keyup paste', rerun);
document.getElementById("input").addEventListener("keydown", function(e) {
    if (e.which===9) {
        e.preventDefault();
        document.execCommand("insertText", false, "\t");
    }
}, false);

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
