from . import rasterize_cpu


def forward_face_index_map_cpu(faces, face_index_map, depth_map,
                               silhouette_image):
    rasterize_cpu.forward_face_index_map(faces, face_index_map, depth_map,
                                         silhouette_image)


def backward_silhouette_cpu(faces, face_vertices, vertices, face_index_map,
                            pixel_map, grad_vertices, grad_silhouette,
                            debug_grad_map):
    rasterize_cpu.backward_silhouette(faces, face_vertices, vertices,
                                      face_index_map, pixel_map, grad_vertices,
                                      grad_silhouette, debug_grad_map)