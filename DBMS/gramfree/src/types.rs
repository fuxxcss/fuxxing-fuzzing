/*
    元数据-sql语句
*/

enum vertex_type{
    VTYPE_metadata,
    VTYPE_statement,
}

enum edge_type{
    ETYPE_s_to_m,
    ETYPE m_to_s,
    ETYPE_m_to_m,
}

enum meta_type{
    MTYPE_view,
    MTYPE_table,
    MTYPE_trigger,
    MTYPE_int,
    MTYPE_str,
}

enum metamap_status{
    MMSTYPE_create,
    MMSTYPE_drop,
    MMSTYPE_keep,
}

struct Vertex<T>{
    v_data:T,
    v_in:Option<Box<Edge>>,
    v_in_num:u64,
    v_out:Option<Box<Edge>>,
    v_out_num:u64,
}

struct Edge{
    e_type:edge_type,
    e_from:(vertex_type,u64),
    e_to:(vertex_type,u64),
    /* 来自同一个vertex */
    e_from_next:Option<Box<Edge>>,
    /* 前往同一个vertex */
    e_to_next:Option<Box<Edge>>,
}

struct Metadata{
    m_type:meta_type,
    /* 不同表可以有相同的列 */
    m_parent:String,
    m_name:String,
}

pub struct Graph{
    meta:Vec<Vertex<Option<Box<Metadata>>>>,
    stmt:Vec<Vertex<String>>,
}

struct Metamap{
    /* 不同表可以有相同的列 */
    /* 元数据hashmap 元组键(meta_parent_str,meta_str) 映射值meta_type*/
    old_map:Option<Box<HashMap<(Option<String>,String),meta_type>>>,
    new_map:Option<Box<HashMap<(Option<String>,String),meta_type>>>,
}
