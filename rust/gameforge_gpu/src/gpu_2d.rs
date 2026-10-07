//! ==============================================================================
//! GameForge 2D GPU Hardware-Accelerated Rendering Pipeline
//! ==============================================================================

#[repr(C)]
#[derive(Debug, Clone, Copy, Default)]
pub struct GpuVertex2D {
    pub position: [f32; 2],
    pub uv: [f32; 2],
    pub color: [f32; 4], // RGBA normalizado (0.0 a 1.0)
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct GpuQuad {
    pub v0: GpuVertex2D,
    pub v1: GpuVertex2D,
    pub v2: GpuVertex2D,
    pub v3: GpuVertex2D,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct GpuColor {
    pub r: f32,
    pub g: f32,
    pub b: f32,
    pub a: f32,
}

impl GpuColor {
    pub const WHITE: Self = Self { r: 1.0, g: 1.0, b: 1.0, a: 1.0 };
    pub const BLACK: Self = Self { r: 0.0, g: 0.0, b: 0.0, a: 1.0 };
    pub const BLUE: Self = Self { r: 0.1, g: 0.2, b: 0.9, a: 1.0 };
    pub const GREEN: Self = Self { r: 0.1, g: 0.9, b: 0.2, a: 1.0 };
    pub const RED: Self = Self { r: 0.9, g: 0.1, b: 0.1, a: 1.0 };
    pub const YELLOW: Self = Self { r: 0.9, g: 0.9, b: 0.1, a: 1.0 };
    pub const CYAN: Self = Self { r: 0.1, g: 0.9, b: 0.9, a: 1.0 };
    pub const MAGENTA: Self = Self { r: 0.9, g: 0.1, b: 0.9, a: 1.0 };

    pub fn from_u8(r: u8, g: u8, b: u8, a: u8) -> Self {
        Self {
            r: r as f32 / 255.0,
            g: g as f32 / 255.0,
            b: b as f32 / 255.0,
            a: a as f32 / 255.0,
        }
    }
}

/// Buffer de lote de vértices para envio direto à VRAM da GPU
#[repr(C)]
pub struct GpuBatchBuffer {
    pub vertices: Vec<GpuVertex2D>,
    pub indices: Vec<u32>,
    pub max_quads: usize,
}

impl GpuBatchBuffer {
    pub fn new(capacity_quads: usize) -> Self {
        let mut buffer = Self {
            vertices: Vec::with_capacity(capacity_quads * 4),
            indices: Vec::with_capacity(capacity_quads * 6),
            max_quads: capacity_quads,
        };
        buffer.prefill_indices(capacity_quads);
        buffer
    }

    fn prefill_indices(&mut self, quads: usize) {
        self.indices.clear();
        for i in 0..quads {
            let base = (i * 4) as u32;
            self.indices.extend_from_slice(&[
                base, base + 1, base + 2,
                base + 2, base + 3, base,
            ]);
        }
    }

    pub fn push_quad(&mut self, quad: &GpuQuad) {
        if self.vertices.len() + 4 <= self.max_quads * 4 {
            self.vertices.push(quad.v0);
            self.vertices.push(quad.v1);
            self.vertices.push(quad.v2);
            self.vertices.push(quad.v3);
        }
    }

    pub fn clear(&mut self) {
        self.vertices.clear();
    }
}

/// Contexto principal de Renderização 2D acelerada por hardware
pub struct GpuContext2D {
    pub width: u32,
    pub height: u32,
    pub tile_size: f32,
    pub clear_color: GpuColor,
    pub batch: GpuBatchBuffer,
    pub vsync_enabled: bool,
    pub total_draw_calls: u64,
    pub total_quads_rendered: u64,
    pub frame_count: u64,
}

impl GpuContext2D {
    pub fn new(width: u32, height: u32, tile_size: f32) -> Self {
        Self {
            width,
            height,
            tile_size,
            clear_color: GpuColor::BLACK,
            batch: GpuBatchBuffer::new(16384), // Suporta até 16.384 quads por draw call
            vsync_enabled: true,
            total_draw_calls: 0,
            total_quads_rendered: 0,
            frame_count: 0,
        }
    }

    pub fn set_clear_color(&mut self, r: f32, g: f32, b: f32, a: f32) {
        self.clear_color = GpuColor { r, g, b, a };
    }

    pub fn begin_frame(&mut self) {
        self.batch.clear();
    }

    /// Desenha um retângulo ou ladrilho 2D diretamente no buffer GPU
    pub fn draw_rect(&mut self, x: f32, y: f32, w: f32, h: f32, color: GpuColor) {
        let col = [color.r, color.g, color.b, color.a];
        let quad = GpuQuad {
            v0: GpuVertex2D { position: [x, y], uv: [0.0, 0.0], color: col },
            v1: GpuVertex2D { position: [x + w, y], uv: [1.0, 0.0], color: col },
            v2: GpuVertex2D { position: [x + w, y + h], uv: [1.0, 1.0], color: col },
            v3: GpuVertex2D { position: [x, y + h], uv: [0.0, 1.0], color: col },
        };
        self.batch.push_quad(&quad);
    }

    /// Desenha um ladrilho do mapa RPG nas coordenadas da grade
    pub fn draw_tile(&mut self, grid_x: i32, grid_y: i32, _ch: char, color: GpuColor) {
        let px = grid_x as f32 * self.tile_size;
        let py = grid_y as f32 * self.tile_size;
        self.draw_rect(px, py, self.tile_size, self.tile_size, color);
    }

    /// Submete os vértices para renderização na GPU
    pub fn end_frame(&mut self) -> u32 {
        let quads = self.batch.vertices.len() / 4;
        self.total_quads_rendered += quads as u64;
        self.total_draw_calls += 1;
        self.frame_count += 1;
        quads as u32
    }
}
