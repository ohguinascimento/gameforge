//! ==============================================================================
//! 2D Spatial Hash Grid (Aceleração de Colisões em Rust)
//! ==============================================================================

use std::collections::HashMap;

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct EntityAabb2D {
    pub id: u32,
    pub x: f32,
    pub y: f32,
    pub width: f32,
    pub height: f32,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct CollisionPair {
    pub id_a: u32,
    pub id_b: u32,
}

pub struct SpatialHashGrid2D {
    pub cell_size: f32,
    grid: HashMap<(i32, i32), Vec<u32>>,
}

impl SpatialHashGrid2D {
    pub fn new(cell_size: f32) -> Self {
        Self {
            cell_size: if cell_size > 0.0 { cell_size } else { 1.0 },
            grid: HashMap::new(),
        }
    }

    pub fn clear(&mut self) {
        self.grid.clear();
    }

    pub fn insert(&mut self, entity: &EntityAabb2D) {
        let cell_x = (entity.x / self.cell_size).floor() as i32;
        let cell_y = (entity.y / self.cell_size).floor() as i32;

        self.grid.entry((cell_x, cell_y))
            .or_insert_with(|| Vec::with_capacity(8))
            .push(entity.id);
    }

    /// Executa broadphase rápido detectando potenciais colisores na mesma célula
    pub fn find_candidate_pairs(&self, entities: &[EntityAabb2D]) -> Vec<CollisionPair> {
        let mut pairs = Vec::with_capacity(entities.len());

        for (cell, entity_ids) in &self.grid {
            if entity_ids.len() < 2 {
                continue;
            }
            for i in 0..entity_ids.len() {
                for j in (i + 1)..entity_ids.len() {
                    let id_a = entity_ids[i];
                    let id_b = entity_ids[j];
                    pairs.push(CollisionPair { id_a, id_b });
                }
            }
        }

        pairs
    }
}
