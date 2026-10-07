pub mod gpu_2d;
pub mod optimizer;
pub mod spatial;

use gpu_2d::{GpuColor, GpuContext2D};
use optimizer::{optimize_bytecode, verify_bytecode_safety};
use spatial::{CollisionPair, EntityAabb2D, SpatialHashGrid2D};
use std::slice;

// ==============================================================================
// 1. C-ABI FFI: 2D GPU Rendering Functions
// ==============================================================================

#[no_mangle]
pub extern "C" fn gameforge_gpu_create(width: u32, height: u32, tile_size: f32) -> *mut GpuContext2D {
    let ctx = Box::new(GpuContext2D::new(width, height, tile_size));
    Box::into_raw(ctx)
}

#[no_mangle]
pub extern "C" fn gameforge_gpu_destroy(ctx: *mut GpuContext2D) {
    if !ctx.is_null() {
        unsafe {
            drop(Box::from_raw(ctx));
        }
    }
}

#[no_mangle]
pub extern "C" fn gameforge_gpu_set_clear_color(ctx: *mut GpuContext2D, r: f32, g: f32, b: f32, a: f32) {
    if let Some(c) = unsafe { ctx.as_mut() } {
        c.set_clear_color(r, g, b, a);
    }
}

#[no_mangle]
pub extern "C" fn gameforge_gpu_begin_frame(ctx: *mut GpuContext2D) {
    if let Some(c) = unsafe { ctx.as_mut() } {
        c.begin_frame();
    }
}

#[no_mangle]
pub extern "C" fn gameforge_gpu_draw_rect(
    ctx: *mut GpuContext2D,
    x: f32, y: f32, w: f32, h: f32,
    r: f32, g: f32, b: f32, a: f32,
) {
    if let Some(c) = unsafe { ctx.as_mut() } {
        c.draw_rect(x, y, w, h, GpuColor { r, g, b, a });
    }
}

#[no_mangle]
pub extern "C" fn gameforge_gpu_draw_tile(
    ctx: *mut GpuContext2D,
    grid_x: i32, grid_y: i32, ch: u8,
    r: f32, g: f32, b: f32, a: f32,
) {
    if let Some(c) = unsafe { ctx.as_mut() } {
        c.draw_tile(grid_x, grid_y, ch as char, GpuColor { r, g, b, a });
    }
}

#[no_mangle]
pub extern "C" fn gameforge_gpu_end_frame(ctx: *mut GpuContext2D) -> u32 {
    if let Some(c) = unsafe { ctx.as_mut() } {
        c.end_frame()
    } else {
        0
    }
}

#[no_mangle]
pub extern "C" fn gameforge_gpu_get_stats(
    ctx: *mut GpuContext2D,
    out_quads: *mut u64,
    out_draw_calls: *mut u64,
    out_frames: *mut u64,
) {
    if let Some(c) = unsafe { ctx.as_ref() } {
        unsafe {
            if !out_quads.is_null() { *out_quads = c.total_quads_rendered; }
            if !out_draw_calls.is_null() { *out_draw_calls = c.total_draw_calls; }
            if !out_frames.is_null() { *out_frames = c.frame_count; }
        }
    }
}

// ==============================================================================
// 2. C-ABI FFI: Bytecode Optimization Pass
// ==============================================================================

#[no_mangle]
pub extern "C" fn gameforge_rust_optimize_bytecode(
    input_bytes: *const u8,
    input_len: usize,
    out_len: *mut usize,
) -> *mut u8 {
    if input_bytes.is_null() || input_len == 0 || out_len.is_null() {
        return std::ptr::null_mut();
    }

    let slice = unsafe { slice::from_raw_parts(input_bytes, input_len) };
    let (mut optimized, _report) = optimize_bytecode(slice);

    unsafe {
        *out_len = optimized.len();
    }

    let ptr = optimized.as_mut_ptr();
    std::mem::forget(optimized);
    ptr
}

#[no_mangle]
pub extern "C" fn gameforge_rust_free_buffer(buf: *mut u8, len: usize) {
    if !buf.is_null() && len > 0 {
        unsafe {
            drop(Vec::from_raw_parts(buf, len, len));
        }
    }
}

#[no_mangle]
pub extern "C" fn gameforge_rust_verify_bytecode(input_bytes: *const u8, input_len: usize) -> bool {
    if input_bytes.is_null() || input_len == 0 {
        return false;
    }
    let slice = unsafe { slice::from_raw_parts(input_bytes, input_len) };
    verify_bytecode_safety(slice)
}

// ==============================================================================
// 3. C-ABI FFI: Spatial Hash Grid Collision Detector
// ==============================================================================

#[no_mangle]
pub extern "C" fn gameforge_rust_spatial_grid_create(cell_size: f32) -> *mut SpatialHashGrid2D {
    let grid = Box::new(SpatialHashGrid2D::new(cell_size));
    Box::into_raw(grid)
}

#[no_mangle]
pub extern "C" fn gameforge_rust_spatial_grid_destroy(grid: *mut SpatialHashGrid2D) {
    if !grid.is_null() {
        unsafe {
            drop(Box::from_raw(grid));
        }
    }
}

#[no_mangle]
pub extern "C" fn gameforge_rust_spatial_grid_insert(
    grid: *mut SpatialHashGrid2D,
    id: u32, x: f32, y: f32, w: f32, h: f32,
) {
    if let Some(g) = unsafe { grid.as_mut() } {
        g.insert(&EntityAabb2D { id, x, y, width: w, height: h });
    }
}

#[no_mangle]
pub extern "C" fn gameforge_rust_spatial_grid_clear(grid: *mut SpatialHashGrid2D) {
    if let Some(g) = unsafe { grid.as_mut() } {
        g.clear();
    }
}
