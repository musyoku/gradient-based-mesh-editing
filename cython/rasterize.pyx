import cython
import numpy as np
cimport numpy as np

cdef extern from "src/rasterize.h":
    void cpp_forward_face_index_map(
        float *face_vertices, 
        int *face_index_map, 
        float *depth_map, 
        int *silhouette_image, 
        int batch_size,
        int num_faces,
        int image_width,
        int Fimage_height);
    void cpp_backward_silhouette(
        int* faces,
        float* face_vertices,
        float* vertices,
        int* face_index_map,
        int* pixel_map,
        float* grad_vertices,
        float* grad_silhouette,
        float* debug_grad_map,
        int batch_size,
        int num_faces,
        int num_vertices,
        int image_width,
        int image_height);


def forward_face_index_map(
        np.ndarray[float, ndim=4, mode="c"] face_vertices not None, 
        np.ndarray[int, ndim=3, mode="c"] face_index_map not None,
        np.ndarray[float, ndim=3, mode="c"] depth_map not None,
        np.ndarray[int, ndim=3, mode="c"] silhouette_image not None):
    batch_size, num_faces = face_vertices.shape[:2]
    image_width, image_height = face_index_map.shape[1:3]
    assert(face_vertices.shape[2] == 3)
    assert(face_vertices.shape[3] == 3)
    assert(face_index_map.shape[1] == depth_map.shape[1])
    assert(face_index_map.shape[2] == depth_map.shape[2])
    assert(face_index_map.shape[1] == silhouette_image.shape[1])
    assert(face_index_map.shape[2] == silhouette_image.shape[2])

    return cpp_forward_face_index_map(
        &face_vertices[0, 0, 0, 0], 
        &face_index_map[0, 0, 0],
        &depth_map[0, 0, 0], 
        &silhouette_image[0, 0, 0], 
        batch_size,
        num_faces,
        image_width,
        image_height)

def backward_silhouette(
        np.ndarray[int, ndim=3, mode="c"] faces not None,
        np.ndarray[float, ndim=4, mode="c"] face_vertices not None, 
        np.ndarray[float, ndim=3, mode="c"] vertices not None,
        np.ndarray[int, ndim=3, mode="c"] face_index_map not None,
        np.ndarray[int, ndim=3, mode="c"] pixel_map not None,
        np.ndarray[float, ndim=3, mode="c"] grad_vertices not None,
        np.ndarray[float, ndim=3, mode="c"] grad_silhouette not None,
        np.ndarray[float, ndim=3, mode="c"] debug_grad_map not None):
    batch_size, num_faces = face_vertices.shape[:2]
    num_vertices = vertices.shape[1]
    image_width, image_height = face_index_map.shape[1:3]
    assert(faces.shape[2] == 3)
    assert(vertices.shape[2] == 3)
    assert(face_vertices.shape[2] == 3)
    assert(face_vertices.shape[3] == 3)

    return cpp_backward_silhouette(
        &faces[0, 0, 0],
        &face_vertices[0, 0, 0, 0], 
        &vertices[0, 0, 0],
        &face_index_map[0, 0, 0], 
        &pixel_map[0, 0, 0], 
        &grad_vertices[0, 0, 0], 
        &grad_silhouette[0, 0, 0], 
        &debug_grad_map[0, 0, 0], 
        batch_size,
        num_faces,
        num_vertices,
        image_width,
        image_height)