pub mod types;
pub use types::Graph;

impl<T> Vertex<T> {
    pub fn new(t:T) -> Self{
        Vectex{
            v_data:t,
            v_in:None,
            v_in_num:0,
            v_out:None,
            v_out_num:0,
        }
    }
}

impl Edge{
    pub fn new(etp:edge_type,efrom:(vertex_type,u64),eto:(vertex_type,u64),efrom_next,eto_next) -> Self{
        Edge{
            e_type:etp,
            e_from:efrom,
            e_to:eto,
            e_from_next:efrom_next,
            e_to_next:eto_next,
        }
    }
}

impl Metadata{
    pub fn new(mtype:meta_type,mname:String,mparent:String) -> Self{
        Metadata{
            m_type:mtype,
            m_name:mname,
            m_parent:mparent,
        }
    }
}

impl Graph{
    pub fn new() -> Self{
        Graph{
            meta:Vec<Vertex<Metadata>>::new(null),
            stmt:Vec<Vertex<String>>::new(""),
        }
    }
    fn copy(& self) -> Graph{
        graph_copy:Graph=Graph::new();
        graph_copy.meta=self.meta;
        graph_copy.stmt=self.stmt;
        return graph_copy;
    }
    fn add_vertex<T>(& mut self,data:T){
        v_new:mut Vertex<T>=Vertex<T>::new(data);
        self.stmt.push(v_new);
    }
    fn add_edge(& mut self,e_type:edge_type,index_from:u64,index_to:u64){
        match e_type{
            ETYPE_s_to_m => {
                let e_from:(vertex_type,u64)=(VTYPE_statement,index_from);
                let e_to:(vertex_type,u64)=(VTYPE_metadata,index_to);
                let e_from_next:Box<Edge>=self.stmt[index_from].v_out;
                let e_to_next:Box<Edge>=self.meta[index_to].v_in;
                let edge:Edge=Edge::new(e_type,e_from,e_to,e_from_next,e_to_next);
                self.stmt[index_from].v_out=edge;
                self.stmt[index_from].v_out_num+=1;
                self.meta[index_to].v_in=edge;
                self.meta[index_to].v_in_num+=1;
            }
            ETYPE_m_to_s => {
                let e_from:(vertex_type,u64)=(VTYPE_metadata,index_from);
                let e_to:(vertex_type,u64)=(VTYPE_statement,index_to);
                let e_from_next:Box<Edge>=self.meta[index_from].v_out;
                let e_to_next:Box<Edge>=self.stmt[index_to].v_in;
                let edge:Edge=Edge::new(e_type,e_from,e_to,e_from_next,e_to_next);
                self.meta[index_from].v_out=edge;
                self.meta[index_from].v_out_num+=1;
                self.stmt[index_to].v_in=edge;
                self.stmt[index_to].v_in_num+=1;
            }
            ETYPE_m_to_m => {
                let e_from:(vertex_type,u64)=(VTYPE_metadata,index_from);
                let e_to:(vertex_type,u64)=(VTYPE_metadata,index_to);
                let e_from_next:Box<Edge>=self.meta[index_from].v_out;
                let e_to_next:Box<Edge>=self.meta[index_to].v_in;
                let edge:Edge=Edge::new(e_type,e_from,e_to,e_from_next,e_to_next);
                self.meta[index_from].v_out=edge;
                self.meta[index_from].v_out_num+=1;
                self.meta[index_to].v_in=edge;
                self.meta[index_to].v_in_num+=1;
            }
        }
    }
    /*
    考虑:
    1.元数据有创建、删除和不变三种状态
    2.不同表有相同的列
    */
    pub fn build(& mut self,stmt_str:String,meta_map:& Metamap){
        /* 第一步，复制图、在其中添加stmt */
        graph_copy:Graph=self.copy();
        graph_copy.add_vertex<String>(stmt_str);
        /* 第二步，建立旧元数据与stmt的m_to_s依赖边 */
        let s_index:u64=graph_copy.stmt.len();
        for (m_index,m) in self.meta.iter().enumerate() {
            if stmt_str.contains(m.v_data.m_name) == true {
                graph_copy.add_edge(ETYPE_m_to_s,m_index,s_index);
            }
        }
        /* 第三步，根据三种情况建立新的依赖 */
        let status:metamap_status=meta_map.check_situation().0;
        let vec:Vec<(meta_type,String,String)>=meta_map.check_situation().1;
        match status {
            MMSTYPE_create => {
                /* 加入新元数据，和s_to_m、m_to_m依赖边*/
                for v in vec.iter(){
                    /* 从无parent的元数据开始 */
                    if v.1 == null{
                        metadata:Box<Metadata>=Box<Metadata>::new(v.0,v.1,v.2);
                        graph_copy.add_vertex<Box<Metadata>>>(metadata);
                        let mut m_new_index:u64=graph_copy.meta.len();
                        graph_copy.add_edge(ETYPE_s_to_m,s_index,m_new_index);
                        /* 处理其child元数据 */
                        for v_child in vec.iter(){
                            if v_child.1 == v.2{
                                metadata_child:Box<Metadata>=Box<Metadata>::new(v.0,v.1,v.2);
                                graph_copy.add_vertex<Box<Metadata>>>(metadata_child);
                                let mut m_new_child_index=graph_copy.meta.len();
                                graph_copy.add_edge(ETYPE_s_to_m,s_index,m_new_child_index);
                                graph_copy.add_edge(ETYPE_m_to_m,m_new_index,m_new_child_index);
                            }
                        }
                    }
                }
            }
            MMSTYPE_drop => {
                /* 加入双向依赖边，相应的s_to_m */
                for v in vec.iter(){
                    for (m_index,m) in self.meta.iter().enumerate(){
                        if m.v_data.m_name == v.2 && m.v_data.m_parent == v.1{
                            graph_copy.add_edge(ETYPE_s_to_m,s_index,m_index);
                        }
                    }
                }
            }
            MMSTYPE_keep => {
                /* 什么都不做 */
            }
        }
        /* 第四步，更新图 */
        self.meta=graph_copy.meta;
        self.stmt=graph_copy.stmt;
    }
}

impl Metamap{
    fn new() -> Self{
        Metamap{
            old_map:HashMap::new();
            new_map:HashMap::new();
        }
    }
    pub fn update(& mut self){
        old_map=new_map;
        new_map.clear();
    }
    pub fn check_situation(& self) -> (metamap_status,Vec<(meta_type,String,String)>){
        let mut key_vec:Vec<meta_type,String,String>=Vec<meta_type,String,String>::new();
        let mut status:bool=false;
        /* key被删除 */
        for (key,value) in self.old_map.keys() {
            if self.new_map.contains_key(& key) == false {
                if status == false {
                    status=true;
                }
                key_vec.push((value,key.0,key.1));
            }
        }
        if status return (MMSTYPE_drop,key_vec);
        /* key被创建 */
        for key in self.new_map.keys() {
            if self.old_map.contains_key(& key) == false {
                if status == false {
                    status=true;
                }
                key_vec.push((value,key.0,key.1));
            }
        }
        if status return (MMSTYPE_create,key_vec);
        /* key不变 */
        return (MMSTYPE_keep,key_vec);
    }
}

