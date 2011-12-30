/*!
 * @file PatchModel.cpp
 * @author 橋本敦史
 * @date Last Change:2011/Nov/01.
 */

#include "PatchModel.h"
#include "cvBlending.h"
#include <fstream>
using namespace mmpl;
using namespace mmpl::image;

PatchModel::PatchModel():layer(&patches),max_id(0){}
PatchModel::~PatchModel(){}

PatchModel::PatchModel(const Image& base_bg):layer(&patches),max_id(0){
	setBaseBG(base_bg);
}

/*
 * @brief 背景画像をセットし、それ以外は全てリセットする
 * */
void PatchModel::setBaseBG(const Image& base_bg){
	this->base_bg = base_bg;
	newest_bg = base_bg;
	hidden_image = base_bg;
	hidden_mask = Image(
			base_bg.getWidth(),
			base_bg.getHeight(),
			IPL_DEPTH_32F,1);
	cvZero(hidden_image.getIplImage());
	cvZero(hidden_mask.getIplImage());
	patches.clear();
	changed_bg = false;
	changed_fg = false;
}


/*
 * @brief 完全に新しいパッチを登録する
 * */
size_t PatchModel::putPatch(const Image& mask, const Image& newest_image){

/*	// まずは、このmask領域が現在移動中と思われるパッチでないか確認する
	std::vector<size_t> matched_ids;
	Image mask(_mask);
	bool hasRest = true;
std::cerr << __FILE__ << ": " << __LINE__ << std::endl;

	for(size_t i=0;i<trackers.size();i++){
		if(!trackers[i].findTrackingTarget(mask,newest_image,newest_bg,&matched_ids,&mask)){
std::cerr << __FILE__ << ": " << __LINE__ << std::endl;
			hasRest = false;
			break;
		}
	}
std::cerr << __FILE__ << ": " << __LINE__ << std::endl;
	// もう対応する物体がない追跡器を削除
	for(size_t i=0;i<trackers.size();i++){
		if(!trackers[i].hasTargets()){
			trackers.erase(trackers.begin()+i);
			i--;
		}
	}
	std::cerr << "num of tracker: " << trackers.size() << std::endl;

	for(size_t i=0;i<matched_ids.size();i++){
		isMoving[matched_ids[i]] = false;
		layer.push(matched_ids[i]);
		// 背景画像を更新する
		// (未実装)
	}
std::cerr << __FILE__ << ": " << __LINE__ << std::endl;

	if(!hasRest){
		// 残ったmask領域がないなら、新たなパッチは登録しない
std::cerr << __FILE__ << ": " << __LINE__ << std::endl;
		return UINT_MAX;
	}
*/

	size_t ID = max_id;
	patches[ID] = Patch(mask,newest_image,newest_bg);
	std::map<size_t,Patch>::iterator ppatch = patches.find(ID);
	if( ppatch->second.getEdgeImage().getWidth() == 1 
		&& ppatch->second.getEdgeImage().getHeight() == 1){
		patches.erase(ppatch);
		return UINT_MAX;
	}

	// 重なったパッチのvisivility maskを更新する
	// パッチの上下関係を更新する
	layer.push(ID);
	std::vector<size_t> lower_patches = layer.getAllBeneathPatch(ID,Patch::original);
	for(size_t i=0;i<lower_patches.size();i++){
		patches[lower_patches[i]].setVisibility(
				ppatch->second.getRect(Patch::original),
				ppatch->second.getMask(Patch::original),
				false);
	}


	put_list.push_back(ID);

	max_id++;
	isMoving.resize(max_id,false);
	changed_fg = true;
	return ID;
}

/*
 * @brief isFloatがtrueだった物体の移動位置を確定させる
 * */
/*
void PatchModel::movePatch(size_t ID,const cv::mat& homography, const Image& newest_image){
	// homographyに従って局所特徴の位置や値を変換
	// newest_imageと局所特徴が類似していたらmask[original]を1.0に。
	// 最後にextract(mask[original])する
	assert(patches[ID].isFloat);
	patches[ID].isFloat = false;
	patches[ID].move(homography,newest_image);

	// パッチの上下関係を更新する
	layer.push(ID,patches[ID]);

	changed = true;
	// 新しいパッチのマスクをupdate_maskに加える
	includeMaskOf(ID,Patch::dilate);
}
*/


/*
 * @brief パッチをモデルから取り除く
 * */
void PatchModel::takePatch(size_t ID,std::vector<size_t>* taken_patch_ids){
	std::map<size_t,Patch>::iterator ppatch;
	ppatch = patches.find(ID);
	assert(patches.end() != ppatch);

	// 既に取り除かれたパッチなら終了
	if(isMoving[ppatch->first]) return;
	isMoving[ppatch->first] = true;

	// 自分より下にある物体のvisibilityをtrueに戻す
	std::vector<size_t> lower_patches = layer.getAllBeneathPatch(ID,Patch::original);
	for(size_t i=0;i<lower_patches.size();i++){
		patches[lower_patches[i]].setVisibility(
				ppatch->second.getRect(Patch::original),
				ppatch->second.getMask(Patch::original),
				true);
	}

	// IDより上にあるパッチを先に取り去る
	size_t upper_patch_id;
	while(UINT_MAX != (upper_patch_id = layer.getUpperPatch(ID,Patch::original))){
		takePatch(upper_patch_id,taken_patch_ids);
	}

	if(taken_patch_ids!=NULL){
		taken_patch_ids->push_back(ID);
	}

	// 自分の後ろの画像を更新用に作成
	hidden_image.setROI(ppatch->second.getRect(Patch::dilate));
	hidden_mask.setROI(ppatch->second.getRect(Patch::dilate));
	// 既にhidden_imageがセットされている領域に対して、
	// hidden_imageとppatchで共通部分がある場合のブレンディングマスクを作る
	Image common_mask = ppatch->second.getMask(Patch::dilate);
	Image temp = common_mask;
	cvThreshold(hidden_mask.getIplImage(),temp.getIplImage(),0.0,1.0,CV_THRESH_BINARY);
	cvSubRS(common_mask.getIplImage(),cvScalar(1.0),common_mask.getIplImage());

	cvMul(common_mask.getIplImage(),temp.getIplImage(),common_mask.getIplImage());

//	cvShowImage("hoge",common_mask.getIplImage());

	cvBlending(
			hidden_image.getIplImage(),
			ppatch->second.getBG(Patch::dilate).getIplImage(),
			common_mask.getIplImage(),
			hidden_image.getIplImage());
	hidden_image.removeROI();
//	cvShowImage("huga",hidden_image.getIplImage());

	// 自分の後ろの画像に対する更新率を設定
	cvMax(
			hidden_mask.getIplImage(),
			ppatch->second.getMask(Patch::dilate).getIplImage(),
			hidden_mask.getIplImage()
			);
	hidden_mask.removeROI();
//	cvShowImage("hage",hidden_mask.getIplImage());
//	cvWaitKey(-1);

	// 自分のIDをlayerから取り除く
	layer.erase(ID);
	// 自分のIDをpatches_in_hiddenから取り除く
	patches_in_hidden.erase(ID);
	changed_bg = true;
}

/*
 * @brief 変更をnewest_bgに反映させる
 * */
void PatchModel::updateNewestBG(){
	// newest_bgを更新

	if(changed_bg){
		changed_bg = false;
		cvBlending(
				hidden_image.getIplImage(),
				newest_bg.getIplImage(),
				hidden_mask.getIplImage(),
				newest_bg.getIplImage());
		cvZero(hidden_image.getIplImage());
		cvZero(hidden_mask.getIplImage());
	}

	if(!changed_fg) return;
	changed_fg = false;
	for(size_t i=0;i<put_list.size();i++){
		std::map<size_t,Patch>::iterator ppatch;
		ppatch = patches.find(put_list[i]);
		assert(patches.end()!=ppatch);
		newest_bg.setROI(ppatch->second.getRect(Patch::dilate));
		cvBlending(
				ppatch->second.getImage(Patch::dilate).getIplImage(),
				newest_bg.getIplImage(),
				ppatch->second.getMask(Patch::dilate).getIplImage(),
				newest_bg.getIplImage());
		newest_bg.removeROI();
	}
	put_list.clear();

}
void PatchModel::setNewestBG(const Image& bg){
	assert(bg.getWidth()==newest_bg.getWidth());
	assert(bg.getHeight()==newest_bg.getHeight());
	assert(bg.getDepth()==newest_bg.getDepth());
	assert(bg.getChannels()==newest_bg.getChannels());
	newest_bg = bg;
}

const Image& PatchModel::getNewestBG()const{
	return newest_bg;
}


size_t PatchModel::checkMovingPatches(const Image& mask,const Image& newest_image,const Image& newest_bg)const{
	assert(mask.getWidth() == base_bg.getWidth());
	assert(mask.getHeight() == base_bg.getHeight());
	assert(mask.getChannels() == 1);
	assert(mask.getDepth() == IPL_DEPTH_8U);

	// maskに包含される領域をチェックして、一番スコアが高いものをmoving_patch_idsに追加
	// 包含されているかどうかのしきい値=0.9
	double min_region_rate = REMOVED_OBJECT_MIN_REGION_RATE;
	double thresh_min_score = REMOVED_OBJECT_MIN_SCORE;

	Patch test_patch(mask,newest_bg,newest_image);
	Image edge = test_patch.getEdgeImage();
	if(edge.getWidth()==1 && edge.getHeight()==1) return UINT_MAX;
	cvSmooth(edge.getIplImage(),edge.getIplImage(),CV_BLUR,4,4);
	cvThreshold(edge.getIplImage(),edge.getIplImage(),0,255,CV_THRESH_BINARY);
	test_patch.setEdgeImage(edge);

	CvRect mrect = test_patch.getRect(Patch::original);
	size_t mask_count = test_patch.getPoints().size();

	for(size_t i=0;i<layer.size();i++){

		std::map<size_t,Patch>::const_iterator ppatch =
			patches.find(layer.getLayer(i,false));
//		std::cerr << "patch id: " << layer.getLayer(i,false) << "/" << layer.size() << std::endl;
		assert(ppatch != patches.end());

		// if tracking moving patch, this works. Otherwise, meaningless.
		if(isMoving[ppatch->first]) continue;

		CvRect patch_rect = ppatch->second.getRect(Patch::original);
		CvRect common_rect = Patch::getCommonRectanglarRegion(
				patch_rect,mrect);
		// 長方形が重なっていなければスキップ
		if(common_rect.width <= 0 || common_rect.height <= 0) continue;


//ppatch->second.save("temp.png",Patch::original,mask.getWidth(),mask.getHeight());
//Image temp("temp.png");

		// パッチの面積(点の数)
		size_t patch_area = ppatch->second.getPoints().size();
		size_t count = 0;
		for(size_t i=0;i<patch_area;i++){
			CvPoint pt = ppatch->second.getPoints()[i];
//			cvCircle(temp.getIplImage(),pt,3,CV_RGB(255,0,0));
			if((unsigned char*)(mask.getIplImage()->imageData + 
						mask.getIplImage()->widthStep * pt.y)[pt.x]!=0){
				count++;
			}
		}

		// 包含されている/しているかどうかのチェック
		double recall_rate = static_cast<double>(count)/patch_area;
		double precision_rate = static_cast<double>(count)/mask_count;
//		std::cerr << "recall_rate: " << recall_rate << std::endl;
//		std::cerr << "precis_rate: " << precision_rate << std::endl;
		if(recall_rate < min_region_rate
				&& precision_rate < min_region_rate) continue;


		// 一致度のチェック
		double score = calcCommonEdge(common_rect,test_patch,ppatch->second);

//		std::cerr << "score: " << score << std::endl;
//		cvWaitKey(-1);

		if(score > thresh_min_score){
			return ppatch->first;
		}
	}

	// No vanished objects are found.
	return UINT_MAX;
}

bool PatchModel::isThere(size_t ID, const Image& newest_image)const{
	std::map<size_t,Patch>::const_iterator ppatch;
	ppatch = patches.find(ID);
	assert(patches.end() != ppatch);
	Image mask(
			newest_image.getWidth(),
			newest_image.getHeight(),
			IPL_DEPTH_8U,
			1);
	cvZero(mask.getIplImage());
	mask.setROI(ppatch->second.getRect(Patch::original));
	cvConvertScale(ppatch->second.getMask(Patch::original).getIplImage(),mask.getIplImage(),255,0);
	mask.removeROI();
	Patch test_patch(mask,newest_image,base_bg);
	Image edge = test_patch.getEdgeImage();
	if(edge.getWidth()==1 && edge.getHeight()==1) return false;
	cvSmooth(edge.getIplImage(),edge.getIplImage(),CV_BLUR,4,4);
	cvThreshold(edge.getIplImage(),edge.getIplImage(),0,255,CV_THRESH_BINARY);
	test_patch.setEdgeImage(edge);

	double score = calcCommonEdge(test_patch.getRect(Patch::original),test_patch,ppatch->second);
	std::cerr << "isThere: score for " << ID << " = " << score << std::endl;
	if(score < IS_THERE_OBJECT_MIN_RECALL_RATE){
		return false;
	}
	return true;
}

void PatchModel::save(
		const std::string& file_head,
		const std::string& ext)const{

	std::stringstream ss;
	int width = base_bg.getWidth();
	int height = base_bg.getHeight();

	std::ofstream fout;
	fout.open((file_head+"_state.txt").c_str());
	assert(fout);

	// パッチの状態をあらわすファイルを作成
	std::map<size_t,Patch>::const_iterator ppatch;
	for(ppatch = patches.begin();
			ppatch != patches.end();ppatch++){
		ss.str("");
		ss << file_head << "_p" << std::setw(2) << std::setfill('0') << ppatch->first;
		ppatch->second.save(ss.str()+"original"+ext,Patch::original,width,height);
		ppatch->second.save(ss.str()+"dilate"+ext,Patch::dilate,width,height);
		fout << ppatch->first << ": ";
		if(isMoving[ppatch->first]){
			fout << "Not on BasePatch" << std::endl;
		}
		else{
			fout << layer.getOrder(ppatch->first) << "th patch" << std::endl;
		}
	}

	fout.close();
}

const Image& PatchModel::getBaseBG()const{
	return base_bg;
}

const Patch& PatchModel::operator[](size_t ID)const{
	std::map<size_t,Patch>::const_iterator pp;
	pp = patches.find(ID);
	assert(pp!=patches.end());
	return pp->second;
}


void PatchModel::getHiddenPatches(const Image& mask,std::vector<size_t>* newly_hidden_patch_ids,std::vector<size_t>* reappeared_patch_ids){
	assert(1 == mask.getChannels());
	assert(NULL!=newly_hidden_patch_ids);
	assert(NULL!=reappeared_patch_ids);
	assert(mask.getHeight()==newest_bg.getHeight());
	assert(mask.getWidth()==newest_bg.getWidth());


	// maskの中の4x4のブロックの数をカウントする
	size_t mask_count = 0;
	CvRect mrect;mrect.x=INT_MAX;mrect.y=INT_MAX;mrect.width=0;mrect.height=0;

	for(int y = 0; y < mask.getHeight(); y += BLOCK_SIZE){
		const unsigned char* pmask = (const unsigned char*)mask.getIplImage()->imageData + y * mask.getIplImage()->widthStep;
		for(int x = 0; x < mask.getWidth(); x += BLOCK_SIZE){
			if(pmask[x]==0) continue;
			mrect.x = mrect.x<x?mrect.x:x;
			mrect.y = mrect.y<y?mrect.y:y;
			mrect.width = mrect.width > x?mrect.width:x;
			mrect.height = mrect.height > y?mrect.height:y;
			mask_count++;
		}
	}


	std::vector<size_t> current_hidden_patches;
	for(std::map<size_t,Patch>::const_iterator ppatch = patches.begin();
			ppatch != patches.end(); ppatch++){
//		if(isMoving[ppatch->first]) continue;

		CvRect patch_rect = ppatch->second.getRect(Patch::original);
		// 長方形が重なっていなければスキップ
		if(patch_rect.x > mrect.width
				|| patch_rect.y > mrect.height
				|| patch_rect.x + patch_rect.width < mrect.x
				|| patch_rect.y + patch_rect.height < mrect.y) continue;

		// パッチの面積(点の数)
		size_t patch_area = ppatch->second.getPoints().size();
		size_t count = 0;
		for(size_t i=0;i<patch_area;i++){
			CvPoint pt = ppatch->second.getPoints()[i];
//			std::cerr << ppatch->first << ": (" << pt.x << ", " << pt.y << ")" << std::endl;
			if(((unsigned char*)(mask.getIplImage()->imageData + 
						mask.getIplImage()->widthStep * pt.y))[pt.x]!=0){
				count++;
			}
		}

		double recall_rate = static_cast<double>(count)/patch_area;
		double min_recall_rate = HIDDEN_OBJECT_MIN_RECALL_RATE;

		if(recall_rate > min_recall_rate){
			current_hidden_patches.push_back(ppatch->first);
//			std::cerr << ppatch->first << " is now hidden." << std::endl;
		}
	}

	newly_hidden_patch_ids->clear();
	reappeared_patch_ids->clear();
	std::set<size_t>::iterator pp;
	std::set<size_t> temp(patches_in_hidden);
	std::set<size_t> next_in_hidden;
	for(size_t i=0;i<current_hidden_patches.size();i++){
		next_in_hidden.insert(current_hidden_patches[i]);
		pp = temp.find(current_hidden_patches[i]);
		if(pp == temp.end()){
			newly_hidden_patch_ids->push_back(current_hidden_patches[i]);
		}
		else{
			temp.erase(pp);
		}
	}
/*
	std::cerr << "prev_hidden_patches   :";
	for(pp = patches_in_hidden.begin();pp != patches_in_hidden.end();pp++){
		std::cerr << *pp << ", ";
	}
	std::cerr << std::endl;

	std::cerr << "current_hidden_patches:";
	for(size_t i=0;i<current_hidden_patches.size();i++){
		std::cerr << current_hidden_patches[i] << ", ";
	}
	std::cerr << std::endl;
	std::cerr << "newly_hidden_patch    :";
	for(size_t i=0;i<newly_hidden_patch_ids->size();i++){
		std::cerr << newly_hidden_patch_ids->at(i) << ", ";
	}
	std::cerr << std::endl;
*/
	for(pp=temp.begin();pp!=temp.end();pp++){
		reappeared_patch_ids->push_back(*pp);
	}
/*	std::cerr << "reappeared_patch      :";
	for(size_t i=0;i<reappeared_patch_ids->size();i++){
		std::cerr << reappeared_patch_ids->at(i) << ", ";
	}
	std::cerr << std::endl;
*/

	patches_in_hidden = next_in_hidden;
}

double PatchModel::calcCommonEdge(
		const CvRect& common_rect,
		const Patch& base,
		const Patch& patch){
//	Image _base_edge(base.getEdgeImage());
//	Image _patch_edge(patch.getEdgeImage());

	CvRect crect = common_rect;
	crect.width += crect.x;
	crect.height += crect.y;

	size_t common_edge_count = 0;

	CvRect brect = base.getRect(Patch::original);
	CvRect prect = patch.getRect(Patch::original);

	for(int y = crect.y; y < crect.height; y++){

		size_t by = y - brect.y;
		const unsigned char* pbase_edge = (const unsigned char*)(
				base.getEdgeImage().getIplImage()->imageData + by * base.getEdgeImage().getIplImage()->widthStep);

		size_t ry = y - prect.y;
		const unsigned char* ppatch_edge = (const unsigned char*)(
				patch.getEdgeImage().getIplImage()->imageData + ry * patch.getEdgeImage().getIplImage()->widthStep);

		for(int x = crect.x;x < crect.width;x++){
			size_t bx = x - brect.x;
			size_t rx = x - prect.x;
			if(ppatch_edge[rx] > 0){
				if(pbase_edge[bx] > 0){
//					cvCircle(_base_edge.getIplImage(),cvPoint(x,y),3,cvScalar(127,0,0));
//					cvCircle(_patch_edge.getIplImage(),cvPoint(rx,ry),3,cvScalar(127,0,0));
					common_edge_count++;
				}
			}
		}
	}

//	cvShowImage("patch_edge",_patch_edge.getIplImage());
//	cvShowImage("base_edge",_base_edge.getIplImage());
	assert(patch.getEdgePixelNum()!=0);
	return static_cast<double>(common_edge_count) / patch.getEdgePixelNum();
}
